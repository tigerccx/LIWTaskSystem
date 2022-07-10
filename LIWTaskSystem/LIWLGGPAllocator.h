#pragma once
/*
* Freelist Allocator with Handle
*/
#include <cstdio>
#include <iostream>
#include <atomic>
#include <mutex>
#include <cassert>

#include "LIWAllocation.h"

// Improvements to make
//TODO: Switch to global handle buffer
//TODO: Could build a TLSF upon this... Can improve lookup speed
//TODO: GC_P2: block wise defrag
//TODO: GC_P3: block return

namespace LIW {
	namespace Util {
		template<size_t SizeTotal, size_t CountHandlePerThread, size_t SizeBlock>
		class LIWLGGPAllocator {
			static_assert(SizeTotal% SizeBlock == 0, "SizeTotal must be multiples of SizeBlock");

		public:
			typedef std::mutex mtx_type;
			typedef std::lock_guard<mtx_type> lkgd_type;
			typedef uintptr_t handle_type;
		public:
			static const size_t c_memSize = SizeTotal;
			static const size_t c_blockSize = SizeBlock;
			static const size_t c_blockCount = SizeTotal / SizeBlock;
			static const size_t c_handleCount = CountHandlePerThread;
			static const uint64_t c_maxAlignment = 16;
		private:
			static const uintptr_t c_idxMax = size_t(-1);
			static const uintptr_t c_idxFull = size_t(-1) - 1;

		private:
			struct SegInfo;
			struct HandleData
			{
				union HandlePtr
				{
					HandleData* next; // Next handle when handle is free
					SegInfo* seg; // Seg it points to when handle is occupied
				} m_ptr;
			};
			struct SegInfo {
				uint64_t m_size{ 0 }; // Available size of seg. (meaning excluding seg info size)
				HandleData* m_handle{ 0 }; // Handle to the seg. NOTE: If null, seg is free
				bool m_mark{ false }; // Seg "to free" mark
			};
			struct BlockInfo {
				size_t m_nextBlock{ c_idxMax }; // Next block (in this local allocator)
				size_t m_nextFreeBlock{ c_idxMax }; // Next free block (in this local allocator)
				size_t m_size{ 0 };		 // Size of this block. (including block info size)
				SegInfo* m_freeSeg{ 0 }; // Pointing to rear available seg.
			};

			static const size_t c_blockInfoSize = sizeof(BlockInfo);
			static const size_t c_segInfoSize = sizeof(SegInfo);
			static const size_t c_handleDataSize = sizeof(HandleData);
			static const size_t c_minSegAllocSize = c_segInfoSize + c_maxAlignment;

			static_assert(SizeBlock > c_blockInfoSize + c_minSegAllocSize, "SizeBlock not large enough");

		private:
			static inline void* OffsetFromSegInfo(SegInfo* seg) {
				return ((char*)seg) + c_segInfoSize;
			}
			static inline void* OffsetFromBlockInfo(BlockInfo* block) {
				return ((char*)block + c_blockInfoSize);
			}

			static inline SegInfo* GetNextSegInfo(SegInfo* seg) {
				return (SegInfo*)((char*)seg + seg->m_size + c_segInfoSize);
			}

		public:
			/// <summary>
			/// Get pointer to allocated space
			/// </summary>
			/// <param name="handle"> handle </param>
			/// <returns> pointer to allocated space </returns>
			static inline void* GetAddressFromHandle(handle_type handle) {
				return OffsetFromSegInfo(((HandleData*)handle)->m_ptr.seg);
			}
			/// <summary>
			/// Set memory value by handle
			/// </summary>
			/// <typeparam name="T"> data type </typeparam>
			/// <param name="handle"> handle </param>
			/// <param name="val"> value to set </param>
			template<class T>
			static inline void SetMem(handle_type handle, T&& val) {
				*((T*)GetAddressFromHandle(handle)) = val;
			}
			template<class T>
			static inline void SetMem(handle_type handle, const T& val) {
				*((T*)GetAddressFromHandle(handle)) = val;
			}
			/// <summary>
			/// Get memory value by handle
			/// </summary>
			/// <typeparam name="T"> data type </typeparam>
			/// <param name="handle"> handle </param>
			/// <returns> value got </returns>
			template<class T>
			static inline T GetMem(handle_type handle) {
				return *((T*)GetAddressFromHandle(handle));
			}

		public:
			class LocalGPAllocator;
			class GlobalGPAllocator {
				friend LocalGPAllocator;

			public:
				/// <summary>
				/// Initialize memory buffer (with alignment). 
				/// </summary>
				inline void Init() {
					size_t align = c_maxAlignment;
					void* const dataBufferRaw = malloc(sizeof(char) * c_memSize + align);
					assert(dataBufferRaw);
					void* const dataBuffer = liw_align_pointer(dataBufferRaw, align);

					m_dataBuffer = (char*)dataBuffer;
					m_dataBufferRaw = (char*)dataBufferRaw;
					assert(m_dataBuffer);
					m_dataBufferEnd = m_dataBuffer + sizeof(char) * c_memSize;
					InitAllBlocks();
				}

				/// <summary>
				/// Get pointer to the block from index. 
				/// </summary>
				/// <param name="idx"> idx to the block </param>
				/// <returns> pointer to the block </returns>
				inline void* GetBlockPtr(size_t idx) {
					return m_dataBuffer + c_blockSize * idx;
				}

				/// <summary>
				/// Fetch block(s) from buffer according to size. 
				/// </summary>
				/// <param name="size"> size required </param>
				/// <returns> idx to the first block </returns>
				size_t FetchBlock(size_t size) {
					int allocBlockCount = 0;
					const size_t allocSize = c_blockSize - c_blockInfoSize - c_segInfoSize;
					for (; allocSize + allocBlockCount * c_blockSize < size; allocBlockCount++) {}
					allocBlockCount += 1;
					{
						lkgd_type lk(m_mtx);
						size_t idxBeg = m_idxAvailableBlock;
						size_t idxBegAlloc = m_idxAvailableBlock;
						size_t idxEnd = m_idxAvailableBlock;
						size_t counter = 0;
						while (counter < c_blockCount) {
							counter++;
							idxEnd = idxBeg;
							int i = 1;
							for (; i < allocBlockCount; i++) { // Search for allocBlockCount of consecutive free blocks
								idxEnd++;
								if (idxEnd == c_blockCount) // Reach the end of all blocks
									break;
								if (!m_availability[idxEnd])
									break;
							}
							if (idxEnd == c_blockCount) { // Reach the end of all blocks
								idxBeg = 0; // Begin from the beginning
							}

							idxBegAlloc = idxBeg;
							idxBeg = idxEnd;
							idxBeg = (idxBeg + 1) % c_blockCount;
							while (idxBeg != idxEnd) // Search for next free block
							{
								if (m_availability[idxBeg]) // Found next free block
									break;
								idxBeg = (idxBeg + 1) % c_blockCount;
							}
							if (idxBeg == idxEnd) { // Exhausted all memory blocks and not one free. 
								throw "Out of mem! All memory allocated! Reduce memory consumption!";
							}
							if (i == allocBlockCount) { // Success
								m_idxAvailableBlock = idxBeg; // Assign a free block idx to m_idxAvailableBlock
								break;
							}
							else // Not enough consecutive blocks 
							{
								// Try again...	
							}
						}

						if (counter == c_blockCount) // Exhausted all memory blocks and not one free. 
							throw "Out of mem! All memory allocated! Reduce memory consumption!";

						// Now that blocks are found...
						idxEnd++;
						for (size_t i = idxBegAlloc; i < idxEnd; i++) { // Make them unavailable
							m_availability[i] = false;
						}
						InitBlock(GetBlockPtr(idxBegAlloc), allocBlockCount * c_blockSize); // Initialize the block
						return idxBegAlloc;
					}
				}

				/// <summary>
				/// Return fetched block(s). 
				/// </summary>
				/// <param name="idxBlock"> idx to the first fetched block </param>
				/// <param name="count"> count of blocks to return </param>
				void ReturnBlock(size_t idxBlock, int count) {
					lkgd_type lk(m_mtx);
					for (size_t idx = idxBlock; idx < idxBlock + count; idx++) {
						m_availability[idx] = true;
					}

					////DEBUG
					//BlockInfo* ptr = GetBlockPtr(idxBlock);
					//size_t size = ptr->m_freeList.m_size;
					//memset(ptr, 0xcdcdcdcdcd, size);
				}

				/// <summary>
				/// Cleanup allocator. 
				/// </summary>
				inline void Cleanup() {
					free(m_dataBufferRaw);
				}

			private:
				char* m_dataBuffer{ nullptr }; // Pointer to allocated space. (aligned)
				char* m_dataBufferEnd{ nullptr }; // Pointer to the end of allocated space. (aligned)
				char* m_dataBufferRaw{ nullptr }; // Pointer to allocated space. (raw)
				size_t m_idxAvailableBlock{ 0 };
				bool m_availability[c_blockCount];
				mtx_type m_mtx;

				/// <summary>
				/// Initialize all blocks
				/// </summary>
				inline void InitAllBlocks() {
					char* ptr = m_dataBuffer;
					char* ptrNext = ptr + c_blockSize;
					for (size_t idx = 0; idx < c_blockCount; idx++, ptr = ptrNext, ptrNext += c_blockSize) { // Set element availabilities
						m_availability[idx] = true; // Mark as free

						////DEBUG
						//memset(ptr, 0xfafafafa, sizeof(uint64_t));
					}
					m_idxAvailableBlock = 0;
				}

				/// <summary>
				/// Initialize a block by creating block info and seg info. 
				/// </summary>
				/// <param name="ptrBlock"> ptr to the block </param>
				/// <param name="size"> size of block </param>
				void InitBlock(void* ptrBlock, size_t size) {
					char* ptrInit = (char*)ptrBlock;
					BlockInfo* ptrBlockInfo = (BlockInfo*)ptrInit;
					SegInfo* ptrSegInfoHead = (SegInfo*)(ptrInit + c_blockInfoSize);
					ptrSegInfoHead->m_size = size - c_blockInfoSize - c_segInfoSize;
					ptrSegInfoHead->m_handle = nullptr;
					ptrSegInfoHead->m_mark = false;
					ptrBlockInfo->m_nextFreeBlock = c_idxMax;
					ptrBlockInfo->m_size = size;
					ptrBlockInfo->m_freeSeg = ptrSegInfoHead;
				}
			};

			class LocalGPAllocator {
			private:
				typedef LIWLGGPAllocator<SizeTotal, CountHandlePerThread, SizeBlock>::GlobalGPAllocator globalAllocator_type;
			public:
				const size_t c_initialSize = 1;

			public:
				inline BlockInfo* GetBlockInfoFromIdx(size_t idx) { return (BlockInfo*)(m_globalAllocator->m_dataBuffer + idx * c_blockSize); }

				/// <summary>
				/// Initialize with a corresponding global allocator. 
				/// </summary>
				/// <param name="globalAllocator"> pointer to a global allocator </param>
				void Init(globalAllocator_type& globalAllocator) {
					m_globalAllocator = &globalAllocator;

					// Init block
					m_idxBlocksFree = m_idxBlocksHead = m_idxBlocksEnd = m_globalAllocator->FetchBlock(c_initialSize);
					BlockInfo* ptrBlockEnd = GetBlockInfoFromIdx(m_idxBlocksEnd);
					ptrBlockEnd->m_nextBlock = c_idxMax;

					// Init handle buffer
					size_t align = sizeof(max_align_t);
					const size_t c_memHandleSize = c_handleDataSize * c_handleCount;
					void* const handleBufferRaw = malloc(sizeof(char) * c_memHandleSize + align);
					assert(handleBufferRaw);
					void* const handleBuffer = liw_align_pointer(handleBufferRaw, align);

					m_handleBuffer = (char*)handleBuffer;
					m_handleBufferEnd = m_handleBuffer + c_memHandleSize;
					m_handleBufferRaw = (char*)handleBufferRaw;

					// Link handle buffer elements
					HandleData* handleCursor = (HandleData*)m_handleBuffer;
					m_handleFree = handleCursor;
					HandleData* handleCursorNext = handleCursor + 1;
					for (size_t i = 0; i < c_handleCount; i++, handleCursor = handleCursorNext, handleCursorNext++) {
						handleCursor->m_ptr.next = handleCursorNext;
					}
				}

				/// <summary>
				/// Cleanup allocator. 
				/// </summary>
				inline void Cleanup() {
					free(m_handleBufferRaw);
				}

				/// <summary>
				/// Allocate a seg of size
				/// </summary>
				/// <param name="size"> size to allocate </param>
				/// <returns> handle pointing to the seg </returns>
				handle_type Allocate(size_t size) {
					size_t idxCursorPrev = c_idxMax;
					size_t idxCursor = m_idxBlocksFree;
					BlockInfo* ptrBlockInfo = nullptr;
					const uint64_t sizeAligned = ((size - 1) / c_maxAlignment + 1) * c_maxAlignment;
					const uint64_t sizeAlloc = sizeAligned + c_segInfoSize;

					if ((uintptr_t)m_handleFree == (uintptr_t)m_handleBufferEnd)
						throw "Out of handles! ";

					//
					// First search in each block
					//
					while (true) {
						if (idxCursor == c_idxMax) { // Ran out of blocks
							//
							// If no seg in no block is suitable...
							//
							size_t idxNewBlock = m_globalAllocator->FetchBlock(sizeAlloc); // Fetch a new block! 

							// Put new block in back
							//TODO:	This policy is intended for a more focused allocation distribution (if possible) and less fragmentation. 
							//		However, this may backfire and slow down allocation. (Let's see)
							BlockInfo* ptrBlockEnd = GetBlockInfoFromIdx(m_idxBlocksEnd);
							ptrBlockEnd->m_nextBlock = idxNewBlock;
							m_idxBlocksEnd = idxNewBlock;
							ptrBlockEnd = GetBlockInfoFromIdx(m_idxBlocksEnd);
							ptrBlockEnd->m_nextBlock = c_idxMax;

							// Add to free list
							BlockInfo* ptrBlockNew = GetBlockInfoFromIdx(idxNewBlock);
							ptrBlockNew->m_nextFreeBlock = m_idxBlocksFree;
							m_idxBlocksFree = idxNewBlock;

							idxCursorPrev = m_idxBlocksEnd;
							idxCursor = idxNewBlock;
						}
						ptrBlockInfo = GetBlockInfoFromIdx(idxCursor);
						SegInfo* const segCur = ptrBlockInfo->m_freeSeg;
						if (segCur->m_size < sizeAligned) { // Seg left in block is not big enough
							idxCursorPrev = idxCursor;
							idxCursor = ptrBlockInfo->m_nextFreeBlock; // Step block
							continue;
						}
						const uint64_t sizeRest = segCur->m_size - sizeAligned;
						if (sizeRest < c_minSegAllocSize) { // There is not enough space for another seg. Just gonna use it. 
							// Assign handle
							HandleData* ptrHandleData = m_handleFree;
							segCur->m_handle = ptrHandleData;
							m_handleFree = ptrHandleData->m_ptr.next;
							ptrHandleData->m_ptr.seg = segCur;

							// No block is left, 
							// so set block info m_freeSeg to nullptr. 
							ptrBlockInfo->m_freeSeg = nullptr;
							// Remove from block freelist
							if (idxCursorPrev == c_idxMax) {
								m_idxBlocksFree = ptrBlockInfo->m_nextFreeBlock;
								ptrBlockInfo->m_nextFreeBlock = c_idxFull; // Mark as full
							}
							else
							{
								BlockInfo* ptrBlockInfoPrev = GetBlockInfoFromIdx(idxCursorPrev);
								ptrBlockInfoPrev->m_nextFreeBlock = ptrBlockInfo->m_nextFreeBlock;
								ptrBlockInfo->m_nextFreeBlock = c_idxFull; // Mark as full
							}
						}
						else { // There is enough space for another seg. Separate and make a new seg. 
							SegInfo* const segNew = (SegInfo*)(((char*)segCur) + sizeAlloc);

							// Process segs list
							segNew->m_size = sizeRest - c_segInfoSize;
							segNew->m_handle = nullptr;
							segNew->m_mark = false;
							segCur->m_size = sizeAligned;

							// Assign handle
							HandleData* ptrHandleData = m_handleFree;
							segCur->m_handle = ptrHandleData;
							m_handleFree = ptrHandleData->m_ptr.next;
							ptrHandleData->m_ptr.seg = segCur;

							// Join new seg into freelist
							ptrBlockInfo->m_freeSeg = segNew;
						}
						return (handle_type)(segCur->m_handle); // Found and returned
					}
				}

				/// <summary>
				/// Free seg which the handle is pointing to
				/// Note: Can be dealing with handles from another thread. 
				/// Note: It is not really freed, but only marked to free. So it won't be available immediately but after GC. 
				/// </summary>
				/// <param name="handle"> handle </param>
				inline void Free(handle_type handle) {
					// Return seg
					SegInfo* segFree = ((HandleData*)handle)->m_ptr.seg;
					segFree->m_mark = true; // Mark for free
					// Handle will be returned when actually freeing seg
				}

				/// <summary>
				/// Garbage collection Pass1. 
				/// NOTE: Need to be called every frame
				/// </summary>
				void GC_P1() {
					//
					// Pass 1
					// Search through blocks, free segs and handles, defrag blocks
					// 
					size_t idxCursor = m_idxBlocksHead;
					BlockInfo* ptrBlockCur = nullptr;
					while (idxCursor < c_idxMax) {
						ptrBlockCur = GetBlockInfoFromIdx(idxCursor);
						SegInfo* const segBlockEnd = (SegInfo*)((char*)ptrBlockCur + ptrBlockCur->m_size);

						SegInfo* segCur = (SegInfo*)OffsetFromBlockInfo(ptrBlockCur);
						SegInfo* segDefragTo = segCur;

						while (segCur < segBlockEnd) {
							if (segCur->m_mark) { // If seg is marked to free, free seg and its handle
								// Free seg
								// (which is delayed to defrag step)
								segCur->m_mark = false;
								// Free handle
								HandleData* handle = segCur->m_handle;
								segCur->m_handle = nullptr;
								handle->m_ptr.next = m_handleFree;
								m_handleFree = handle;

								////DEBUG
								//memset(OffsetFromSegInfo(segCur), 0xffffffff, segCur->m_size);
							}

							const size_t sizeSeg = segCur->m_size + c_segInfoSize;
							if (segCur->m_handle) { // If handle is not null, seg is not free
								// Move memory and defrag
								if (segCur != segDefragTo) {
									memmove_s(segDefragTo, sizeSeg, segCur, sizeSeg);
									// Asjust handle
									HandleData* const handle = segDefragTo->m_handle;
									handle->m_ptr.seg = segDefragTo;
								}
								// Offset segDefragTo
								segDefragTo = (SegInfo*)((char*)segDefragTo + sizeSeg);
							}

							segCur = (SegInfo*)((char*)segCur + sizeSeg); // Step seg. NOTE: Seg could have been moved, so cannot use GetNextSegInfo
						}

						// Now that all defrag in a block is done, 
						// segDefragTo should point to the beg of the free space left in block,
						// or pointing to end pointer since there is no space in block anyway. 
						if (segDefragTo < segBlockEnd) {
							// If there is space, gonna create a seg for the rest of the block space. 
							// NOTE: since every seg ever created is big enough for containing a seg, so no size check is required here. 
							segDefragTo->m_size = (uintptr_t)segBlockEnd - (uintptr_t)segDefragTo - c_segInfoSize;
							segDefragTo->m_handle = nullptr;
							segDefragTo->m_mark = false;
							ptrBlockCur->m_freeSeg = segDefragTo;

							if (ptrBlockCur->m_nextFreeBlock == c_idxFull) {// If the block is not in freelist
								// Add back to free list
								ptrBlockCur->m_nextFreeBlock = m_idxBlocksFree;
								m_idxBlocksFree = idxCursor;
							}

							////DEBUG
							//memset(OffsetFromSegInfo(segDefragTo), 0xffffffff, segDefragTo->m_size);
						}


						idxCursor = ptrBlockCur->m_nextBlock; // Step block
					}
				}

				/*void GC_P2() {
					//
					// Pass 2
					// Search through blocks, concentrate data in front blocks, further defrag blocks
					//
					size_t idxCursorPrev = m_idxBlocksFree;
					if (idxCursorPrev == c_idxMax) // If there is no free blocks on thread, there's no need to do this then.
						return;
					BlockInfo* ptrBlockPrev = GetBlockInfoFromIdx(idxCursorPrev);
					size_t sizeFreePrev = ptrBlockPrev->m_freeSeg->m_size + c_segInfoSize;
					while (ptrBlockPrev->m_size - c_blockInfoSize == sizeFreePrev) { // Skip the blocks which are not occupied at all.
						idxCursorPrev = ptrBlockPrev->m_nextFreeBlock;
						ptrBlockPrev = GetBlockInfoFromIdx(idxCursorPrev);
						sizeFreePrev = ptrBlockPrev->m_freeSeg->m_size + c_segInfoSize;
					}

					size_t idxCursorCur = ptrBlockPrev->m_nextFreeBlock;
					if (idxCursorCur == c_idxMax) // If there is only one free block on thread, welp, again, there's no need to do this.
						return;
					BlockInfo* ptrBlockCur = nullptr;

					while (idxCursorCur < c_idxMax) {
						ptrBlockCur = GetBlockInfoFromIdx(idxCursorCur);

						SegInfo* segFreePrev = ptrBlockPrev->m_freeSeg;
						SegInfo* segFree = ptrBlockCur->m_freeSeg;
						const size_t sizeBlock = ptrBlockCur->m_size;
						size_t sizeFree = segFree->m_size + c_segInfoSize;
						size_t sizeOccupied = sizeBlock - c_blockInfoSize - sizeFree;

						if (sizeOccupied != 0) { // If there is space occupied
							if (sizeOccupied < sizeFreePrev) { // Prev free block can hold everything in cur free block.
														   // That's Great!
								// Move everything!
								SegInfo* segCur = (SegInfo*)OffsetFromBlockInfo(ptrBlockCur);
								memmove_s(segFreePrev, sizeFreePrev, segCur, sizeOccupied);
								SegInfo* const segEnd = (SegInfo*)((char*)segCur + sizeOccupied);
								segCur = segFreePrev;
								SegInfo* segNext = GetNextSegInfo(segCur);
								while (segNext < segEnd) { // Adjust handles for each moved seg
									HandleData* const handle = segCur->m_handle;
									handle->m_ptr.seg = segCur;
									segCur = segNext;
									segNext = GetNextSegInfo(segNext);
								}
								// One handle left
								HandleData* const handle = segCur->m_handle;
								handle->m_ptr.seg = segCur;

								size_t sizeRest = sizeFreePrev - sizeOccupied;
								if (sizeRest < c_minSegAllocSize) { // Oh no! The memory left in block is not enough for another allocation whatsoever!
																	// Fine... Let's merge that into the last allocated seg.
									segCur->m_size += sizeRest;

									// Step
									// Step cur block and prev block
									idxCursorPrev = idxCursorCur;
									ptrBlockPrev = ptrBlockCur;
									sizeFreePrev = ptrBlockPrev->m_freeSeg->m_size + c_segInfoSize;
									while (ptrBlockPrev->m_size - c_blockInfoSize == sizeFreePrev) { // Skip the blocks which are not occupied at all.
										idxCursorPrev = ptrBlockPrev->m_nextFreeBlock;
										ptrBlockPrev = GetBlockInfoFromIdx(idxCursorPrev);
										sizeFreePrev = ptrBlockPrev->m_freeSeg->m_size + c_segInfoSize;
									}
									idxCursorCur = ptrBlockPrev->m_nextFreeBlock;
								}
								else // Noice! Still got memory enough for at least one allocation.
								{
									segNext->m_size = sizeRest - c_segInfoSize;
									segNext->m_handle = nullptr;
									segNext->mark = false;

									// Step
									// Only step cur block since prev block still has space
									idxCursorCur = ptrBlockCur->m_nextFreeBlock;
								}
							}

							else // Prev free block cannot hold everything in cur free block.
								 // Fine... Let's do this one by one.
							{
								SegInfo* segBeg = (SegInfo*)OffsetFromBlockInfo(ptrBlockCur);
								SegInfo* segEnd = (SegInfo*)((char*)segCur + sizeOccupied);
								const size_t sizeDstTotal = segFreePrev->m_size + c_segInfoSize;
								SegInfo* const segMax = (SegInfo*)((char*)segCur + sizeDstTotal);

								SegInfo* segCur = segBeg;
								SegInfo* segNext = GetNextSegInfo(segCur);

								while (segNext < segMax) { // Adjust handles for each seg to move
									HandleData* const handle = segCur->m_handle;
									ptrdiff_t const offset = (uintptr_t)segCur - (uintptr_t)segBeg;
									handle->m_ptr.seg = (SegInfo*)((char*)segFreePrev + offset);
									segCur = segNext;
									segNext = GetNextSegInfo(segNext);
								}
								// One handle left
								HandleData* const handle = segCur->m_handle;
								ptrdiff_t const offset = (uintptr_t)segCur - (uintptr_t)segBeg;
								size_t const sizeTotal = offset + segCur->m_size + c_segInfoSize;
								SegInfo* const segCur1 = (SegInfo*)((char*)segFreePrev + offset);
								handle->m_ptr.seg = segCur;
								SegInfo* const segNext1 = (SegInfo*)((char*)segFreePrev + sizeTotal);

								// Move to BlockPrev
								memmove_s(segFreePrev, sizeFreePrev, segBeg, sizeTotal);

								size_t const sizeRest = sizeFreePrev - sizeTotal;
								if (sizeRest < c_minSegAllocSize) { // Oh no! The memory left in block is not enough for another allocation whatsoever!
																	// Fine... Let's merge that into the last allocated seg.
									segCur1->m_size += sizeRest;
								}
								else // Noice! Still got memory enough for at least one allocation.
								{
									segNext1->m_size = sizeRest - c_segInfoSize;
									segNext1->m_handle = nullptr;
									segNext1->mark = false;
								}

								// Move the rest to block front
								size_t const sizeOccupiedRest = sizeOccupied - sizeTotal;
								memmove_s(segBeg, sizeBlock - c_blockInfoSize, segBeg, sizeOccupiedRest + c_segInfoSize);
								segCur = segBeg;
								segEnd = (SegInfo*)((char*)segBeg + sizeOccupiedRest);
								while (segCur < segEnd) { // Adjust handles for each seg to move
									HandleData* const handle = segCur->m_handle;
									handle->m_ptr.seg = segCur;
									segCur = GetNextSegInfo(segNext);
								}
								// Add freed up space to the last free seg
								segCur->m_size += sizeTotal;

								// Step
								// Step cur block and prev block
								idxCursorPrev = idxCursorCur;
								ptrBlockPrev = ptrBlockCur;
								sizeFreePrev = ptrBlockPrev->m_freeSeg->m_size + c_segInfoSize;
								while (ptrBlockPrev->m_size - c_blockInfoSize == sizeFreePrev) { // Skip the blocks which are not occupied at all.
									idxCursorPrev = ptrBlockPrev->m_nextFreeBlock;
									ptrBlockPrev = GetBlockInfoFromIdx(idxCursorPrev);
									sizeFreePrev = ptrBlockPrev->m_freeSeg->m_size + c_segInfoSize;
								}
								idxCursorCur = ptrBlockPrev->m_nextFreeBlock;
							}
						}
						else { // If there no space is occupied
							   // Step cur block
							idxCursorCur = ptrBlockCur->m_nextFreeBlock;
						}
					}
				}*/

			private:
				size_t m_idxBlocksHead{ c_idxMax }; // Idx to the first block. 
				size_t m_idxBlocksFree{ c_idxMax }; // Idx to the first free block. 
				size_t m_idxBlocksEnd{ c_idxMax }; // Idx to the last block. 
				globalAllocator_type* m_globalAllocator{ nullptr };  // Reference to its global allocator. 

				char* m_handleBuffer{ nullptr }; // Pointer to allocated space for handle. (aligned)
				char* m_handleBufferEnd{ nullptr }; // Pointer to the end of allocated space for handle. (aligned)
				char* m_handleBufferRaw{ nullptr }; // Pointer to allocated space for handle. (raw)
				HandleData* m_handleFree{ nullptr }; // Pointer to the first free handle. 
			};
		};
	}
}


