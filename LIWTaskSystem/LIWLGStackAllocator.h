#pragma once
/*
* 
*/
#include <cstdio>
#include <atomic>
#include <vector>

#include "LIWAllocation.h"

namespace LIW {
	namespace Util {
		template<size_t SizeTotal, size_t SizeBlock>
		class LIWLGStackAllocator {
		
		public:
			static const size_t c_totalSize = SizeTotal;
			static const size_t c_blockSize = SizeBlock;
		public:
			class LocalStackAllocator;
			class GlobalStackAllocator {
				friend class LocalStackAllocator;
			public:
				/// <summary>
				/// Initialize memory buffer (with alignment). 
				/// </summary>
				void Init() {
					size_t align = sizeof(max_align_t);
					void* const dataBufferRaw = malloc(c_totalSize + align);
					assert(dataBufferRaw);
					void* const dataBuffer = liw_align_pointer(dataBufferRaw, align);

					m_dataBuffer = (char*)dataBuffer;
					m_dataBufferEnd = (char*)((uintptr_t)m_dataBuffer + c_totalSize);
					m_dataBufferRaw = (char*)dataBufferRaw;
					m_shift = (uintptr_t)dataBuffer - (uintptr_t)dataBufferRaw;
					m_ptrTop = m_dataBuffer;
				}

				/// <summary>
				/// Allocate blocks of SizeBlock. 
				/// </summary>
				/// <param name="count"> number of blocks to allocate </param>
				/// <returns> allocated block start pointer </returns>
				inline void* AllocateBlocks(size_t count) {
					assert((uintptr_t)m_ptrTop.load(std::memory_order_relaxed) < (uintptr_t)m_dataBufferEnd); // Check stack top pointer still usable
					return (void*)m_ptrTop.fetch_add(c_blockSize * count, std::memory_order_relaxed); // Return the next block
				}

				/// <summary>
				/// Clear allocated blocks. 
				/// </summary>
				inline void Clear() {
					//memset(m_dataBuffer, 0, sizeof(char) * c_totalSize);
					m_ptrTop = m_dataBuffer;
				}

				/// <summary>
				/// Cleanup allocator. 
				/// </summary>
				inline void Cleanup() {
					free(m_dataBufferRaw);
				}
			private:
				char* m_dataBuffer			{ nullptr }; // Pointer to allocated space. (aligned)
				char* m_dataBufferEnd		{ nullptr }; // Pointer to the end of allocated space. (aligned)
				char* m_dataBufferRaw		{ nullptr }; // Pointer to allocated space. (raw)
				ptrdiff_t m_shift			{ 0 }; // Shift from raw. 
				std::atomic<char*> m_ptrTop	{ nullptr }; // Pointer to top of stack. 
			};

			class LocalStackAllocator {
			public:
				typedef LIWLGStackAllocator<SizeTotal, SizeBlock>::GlobalStackAllocator globalAllocator_type;
			public:
				/// <summary>
				/// Initialize with a corresponding global allocator. 
				/// </summary>
				/// <param name="globalAllocator"> pointer to a global allocator </param>
				inline void Init(globalAllocator_type& globalAllocator) {
					m_globalAllocator = &globalAllocator;
					m_dataBuffer = (char*)m_globalAllocator->AllocateBlocks(1);
					m_ptrTop = m_dataBuffer;
				}

				/// <summary>
				/// Allocate a size of memory. 
				/// </summary>
				/// <param name="size"> size of memory to allocate </param>
				/// <returns> allocated memory start pointer </returns>
				inline void* Allocate(size_t size) {
					size_t tmp = m_ptrTop - m_dataBuffer;
					if (m_ptrTop - m_dataBuffer + size > c_blockSize) {
						size_t blockCount = (size + c_blockSize - 1) / c_blockSize;
						m_dataBuffer = (char*)m_globalAllocator->AllocateBlocks(blockCount); // Fetch block(s) from global allocator (which will incur contention)
						m_ptrTop = m_dataBuffer; // Reset marker (Note: could cause memory waste in the last block)
					}
					char* m_ptrTopTmp = m_ptrTop;
					m_ptrTop += size;
					return m_ptrTopTmp;
				}
			private:
				char* m_dataBuffer						{ nullptr }; // Pointer to allocated space. 
				char* m_ptrTop							{ nullptr }; // Pointer to the end of allocated space. 
				globalAllocator_type* m_globalAllocator	{ nullptr }; // Pointer to the global allocator.
			};
		};
	}
}
