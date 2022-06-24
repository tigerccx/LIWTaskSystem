#pragma once
#include <cstdio>
#include <atomic>
#include <mutex>
#include <cassert>

#include "LIWAllocation.h"

namespace LIW {
	namespace Util {
		template<size_t SizeElement, class IndexType, size_t CountElementPerBlock, size_t CountBlock>
		class LIWLGPoolAllocator {
			static_assert(size_t(1) << (sizeof(IndexType) * 8) > CountElementPerBlock * CountBlock, "IndexType too small for pool of this size");
			static_assert(SizeElement >= sizeof(IndexType), "SizeElement too small for storing index");

		public:
			typedef IndexType idx_type;
			typedef std::mutex mtx_type;
			typedef std::lock_guard<mtx_type> lkgd_type;
		public:
			static const size_t c_elementSize	= SizeElement;
			static const size_t c_countPerBlock	= CountElementPerBlock;
			static const size_t c_countBlock	= CountBlock;
			static const size_t c_blockSize		= c_elementSize * c_countPerBlock;
			static const size_t c_countPerPool	= c_countPerBlock * c_countBlock;
			static const size_t c_poolSize		= c_blockSize * c_countBlock;
		public:
			class LocalPoolAllocator;
			class GlobalPoolAllocator {
				friend class LocalPoolAllocator;
			public:
				/// <summary>
				/// Initialize memory buffer (with alignment). 
				/// </summary>
				inline void Init() {
					size_t align = sizeof(max_align_t);
					void* const dataBufferRaw = malloc(sizeof(char) * c_poolSize + align);
					assert(dataBufferRaw);
					void* const dataBuffer = liw_align_pointer(dataBufferRaw, align);

					m_dataBuffer = (char*)dataBuffer;
					m_dataBufferRaw = (char*)dataBufferRaw;
					assert(m_dataBuffer);
					InitAllBlocks();
				}

				/// <summary>
				/// Fetch a block. 
				/// </summary>
				/// <returns> fetched block start pointer </returns>
				void* FetchBlock() {
					lkgd_type lk(m_mtx);
					assert(m_availableList != m_dataBufferEnd); // No available block in pool
					void* ptr = m_availableList;
					const idx_type idxNext = *((idx_type*)ptr);
					m_availableList = m_dataBuffer + idxNext * c_elementSize;
					InitBlock(ptr);
					return ptr;
				}

				/// <summary>
				/// Return a fetched block. 
				/// </summary>
				/// <param name="ptrBlock"> pointer to the fetched block </param>
				void ReturnBlock(void* ptrBlock) {
					lkgd_type lk(m_mtx);
					const ptrdiff_t offset = (uintptr_t)m_availableList - (uintptr_t)m_dataBuffer;
					const idx_type idxOffset = offset / c_elementSize;
					assert(idxOffset * c_elementSize == offset); // First available element doesn't align with the pool
					//memset(ptrBlock, 0, c_blockSize);
					*((idx_type*)ptrBlock) = idxOffset;
					m_availableList = (char*)ptrBlock;
				}

				/// <summary>
				/// Clear allocated blocks. 
				/// </summary>
				inline void Clear() {
					InitAllBlocks();
				}

				/// <summary>
				/// Cleanup allocator. 
				/// </summary>
				inline void Cleanup() {
					free(m_dataBufferRaw);
				}

			private:
				char* m_dataBuffer		{ nullptr }; // Pointer to allocated space. (aligned)
				char* m_dataBufferEnd	{ nullptr }; // Pointer to the end of allocated space. (aligned)
				char* m_dataBufferRaw	{ nullptr }; // Pointer to allocated space. (raw)
				char* m_availableList	{ nullptr }; // Pointer to the first available block. 
				mtx_type m_mtx;

				/// <summary>
				/// Initialize a block by linking the elements in the block. 
				/// </summary>
				/// <param name="ptrBlock"> pointer to the block </param>
				void InitBlock(void* ptrBlock) {
					char* ptrInit = (char*)ptrBlock;
					const uintptr_t offsetByte = ((uintptr_t)ptrInit - (uintptr_t)m_dataBuffer);
					const idx_type idxOffset = (idx_type)(offsetByte / c_elementSize);
					assert(idxOffset * c_elementSize == offsetByte); // ptrBlock is valid
					const idx_type idxBlockEnd = idxOffset + c_countPerBlock;
					assert(idxBlockEnd <= c_poolSize); // Block doesnt exceed max capacity
					for (idx_type idx = idxOffset; idx < idxBlockEnd - 1; ++idx) { // Link elements
						*((idx_type*)ptrInit) = idx + 1;
						ptrInit += c_elementSize;
					}
					*((idx_type*)ptrInit) = c_countPerPool; // The last of the block points to end pointer. 
				}

				/// <summary>
				/// Initialize all blocks by linking the blocks. 
				/// </summary>
				inline void InitAllBlocks() {
					char* ptrInit = m_dataBuffer;
					for (idx_type idxElement = 0; idxElement < c_countPerPool; idxElement += c_countPerBlock) { // Link PoolSize-1 elements
						*((idx_type*)ptrInit) = idxElement + c_countPerBlock;
						ptrInit += c_blockSize;
					}
					m_dataBufferEnd = ptrInit;
					m_availableList = m_dataBuffer;
				}
			};

			class LocalPoolAllocator {
			private:
				typedef LIWLGPoolAllocator<SizeElement, IndexType, CountElementPerBlock, CountBlock>::GlobalPoolAllocator globalAllocator_type;
			public:
				/// <summary>
				/// Initialize with a corresponding global allocator. 
				/// </summary>
				/// <param name="globalAllocator"> pointer to a global allocator </param>
				inline void Init(globalAllocator_type& globalAllocator) {
					m_globalAllocator = &globalAllocator;
					m_availableList = (char*)m_globalAllocator->FetchBlock();
				}

				/// <summary>
				/// Fetch an element from the pool. 
				/// </summary>
				/// <returns> pointer to the element </returns>
				void* Fetch() {
					if (m_availableList == m_globalAllocator->m_dataBufferEnd) { // Fetch new block from global allocator
						m_availableList = (char*)m_globalAllocator->FetchBlock();
					}
					void* const ptr = m_availableList;
					const idx_type idxNext = *((idx_type*)ptr);
					m_availableList = m_globalAllocator->m_dataBuffer + idxNext * c_elementSize;
					return ptr;
				}

				/// <summary>
				/// Return an element to the pool. 
				/// </summary>
				/// <param name="ptr"> pointer to the element </param>
				void Return(void* ptr) {
					const ptrdiff_t offset = (uintptr_t)m_availableList - (uintptr_t)(m_globalAllocator->m_dataBuffer);
					const idx_type offsetIdx = (idx_type)(offset / c_elementSize);
					assert(offsetIdx * c_elementSize == offset); // The first available element in available list doesn't align with the pool
					*((idx_type*)ptr) = offsetIdx;
					m_availableList = (char*)ptr;
				}

				/// <summary>
				/// Reset allocator. 
				/// </summary>
				inline void Reset() {
					m_availableList = (char*)m_globalAllocator->FetchBlock();
				}

			private:
				char* m_availableList{ nullptr };
				globalAllocator_type* m_globalAllocator{ nullptr };
			};
		};
	}
}