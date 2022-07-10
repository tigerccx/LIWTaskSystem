#pragma once
#include <memory>
#include "LIWLGStackAllocator.h"
#include "LIWLGGPAllocator.h"

typedef size_t liw_memory_size_type;

inline const liw_memory_size_type operator""_KB(liw_memory_size_type const x) { return 1024 * x; }
inline const liw_memory_size_type operator""_MB(liw_memory_size_type const x) { return 1024 * 1024 * x; }
inline const liw_memory_size_type operator""_GB(liw_memory_size_type const x) { return 1024 * 1024 * 1024 * x; }


/*
* liw_minit_MODE:			initialize (once per run)
* liw_mupdate_MODE:			update (once per post frame)
* liw_mclnup_MODE:			cleanup (once per run)
* liw_minit_MODE_thd:		initialize (once per run per thread)
* liw_mupdate_MODE_thd:		update (once per post frame per thread)
* liw_mclnup_MODE_thd:		cleanup (once per run per thread)
* liw_malloc_MODE:			malloc
* liw_free_MODE:			free
* liw_new_MODE:				new
* liw_delete_MODE:			delete
*/


//
// System memory allocation
// 

inline void* liw_malloc_sys(liw_memory_size_type size) {
	return malloc(size);
}

inline void liw_free_sys(void* ptr) {
	free(ptr);
}

template<class T, class ... Args>
inline T* liw_new_sys(Args&&... args) {
	T* ptr = (T*)liw_malloc_sys(sizeof(T));
	return new(ptr)T(std::forward<Args>(args)...);
}

template<class T>
inline void liw_delete_sys(T* ptr) {
	ptr->~T();
	liw_free_sys(ptr);
}


//
// Default memory allocation
// 

const size_t SIZE_MEM_DEFAULT_BUFFER = size_t{ 1 } << 32; // 4GB
const size_t COUNT_MEM_DEFAULT_HANDLE = size_t{ 1 } << 20; // around 1M handles each thread
const size_t SIZE_MEM_DEFAULT_BUFFER_BLOCK = size_t{ 1 } << 24; // 16MB
typedef LIW::Util::LIWLGGPAllocator<SIZE_MEM_DEFAULT_BUFFER, COUNT_MEM_DEFAULT_HANDLE, SIZE_MEM_DEFAULT_BUFFER_BLOCK> DefaultBufferAllocator;
struct DefaultMemBuffer
{
	static DefaultBufferAllocator::GlobalGPAllocator s_defaultBufferGAllocator;
	static thread_local DefaultBufferAllocator::LocalGPAllocator tl_defaultBufferLAllocator;
};
DefaultBufferAllocator::GlobalGPAllocator DefaultMemBuffer::s_defaultBufferGAllocator;
thread_local DefaultBufferAllocator::LocalGPAllocator DefaultMemBuffer::tl_defaultBufferLAllocator;

inline void liw_minit_def() {
	DefaultMemBuffer::s_defaultBufferGAllocator.Init();
}

inline void liw_mupdate_def() {

}

inline void liw_mclnup_def() {
	DefaultMemBuffer::s_defaultBufferGAllocator.Cleanup();
}

inline void liw_minit_def_thd() {

	DefaultMemBuffer::tl_defaultBufferLAllocator.Init(DefaultMemBuffer::s_defaultBufferGAllocator);
}

inline void liw_mupdate_def_thd() {
	DefaultMemBuffer::tl_defaultBufferLAllocator.GC_P1();
}

inline void liw_mclnup_def_thd() {
	DefaultMemBuffer::tl_defaultBufferLAllocator.Cleanup();
}

inline void* liw_malloc_def(liw_memory_size_type size) {
	return malloc(size);
}

inline void liw_free_def(void* ptr) {
	free(ptr);
}

template<class T, class ... Args>
inline T* liw_new_def(Args&&... args) {
	T* ptr = (T*)liw_malloc_def(sizeof(T));
	return new(ptr)T(std::forward<Args>(args)...);
}

template<class T>
inline void liw_delete_def(T* ptr) {
	ptr->~T();
	liw_free_def(ptr);
}


//
// Static buffer
//

const size_t SIZE_MEM_STATIC_BUFFER = size_t{ 1 } << 28; // 256MB
const size_t SIZE_MEM_STATIC_BUFFER_BLOCK = size_t{ 1 } << 16; // 64KB
typedef LIW::Util::LIWLGStackAllocator<SIZE_MEM_STATIC_BUFFER, SIZE_MEM_STATIC_BUFFER_BLOCK> StaticBufferAllocator;
struct StaticMemBuffer
{
	static StaticBufferAllocator::GlobalStackAllocator s_staticBufferGAllocator;
	static thread_local StaticBufferAllocator::LocalStackAllocator tl_staticBufferLAllocator;
};
StaticBufferAllocator::GlobalStackAllocator StaticMemBuffer::s_staticBufferGAllocator;
thread_local StaticBufferAllocator::LocalStackAllocator StaticMemBuffer::tl_staticBufferLAllocator;

inline void liw_minit_static() {
	StaticMemBuffer::s_staticBufferGAllocator.Init();
}

inline void liw_mupdate_static() {
	
}

inline void liw_mclnup_static() {
	StaticMemBuffer::s_staticBufferGAllocator.Cleanup();
}

inline void liw_minit_static_thd() {

	StaticMemBuffer::tl_staticBufferLAllocator.Init(StaticMemBuffer::s_staticBufferGAllocator);
}

inline void liw_mupdate_static_thd() {

}

inline void liw_mclnup_static_thd() {

}

inline void* liw_malloc_static(liw_memory_size_type size) {
	return StaticMemBuffer::tl_staticBufferLAllocator.Allocate(size);
}

inline void liw_free_static(void* ptr) {

}

template<class T, class ... Args>
inline T* liw_new_static(Args&&... args) {
	T* ptr = (T*)liw_malloc_static(sizeof(T));
	return new(ptr)T(std::forward<Args>(args)...);
}

template<class T>
inline void liw_delete_static(T* ptr) {
	ptr->~T();
	liw_free_static(ptr);
}


//
// Per frame buffer
//

const size_t SIZE_MEM_FRAME_BUFFER = size_t{ 1 } << 27; // 128MB
const size_t SIZE_MEM_FRAME_BUFFER_BLOCK = size_t{ 1 } << 16; // 64KB
typedef LIW::Util::LIWLGStackAllocator<SIZE_MEM_FRAME_BUFFER, SIZE_MEM_FRAME_BUFFER_BLOCK> FrameBufferAllocator;
struct FrameMemBuffer
{
	static FrameBufferAllocator::GlobalStackAllocator s_frameBufferGAllocator;
	static thread_local FrameBufferAllocator::LocalStackAllocator tl_frameBufferLAllocator;
};
FrameBufferAllocator::GlobalStackAllocator FrameMemBuffer::s_frameBufferGAllocator;
thread_local FrameBufferAllocator::LocalStackAllocator FrameMemBuffer::tl_frameBufferLAllocator;

inline void liw_minit_frame() {
	FrameMemBuffer::s_frameBufferGAllocator.Init();
}

inline void liw_mupdate_frame() {
	FrameMemBuffer::s_frameBufferGAllocator.Clear();
}

inline void liw_mclnup_frame() {
	FrameMemBuffer::s_frameBufferGAllocator.Cleanup();
}

inline void liw_minit_frame_thd() {
	
	FrameMemBuffer::tl_frameBufferLAllocator.Init(FrameMemBuffer::s_frameBufferGAllocator);
}

inline void liw_mupdate_frame_thd() {

}

inline void liw_mclnup_frame_thd() {

}

inline void* liw_malloc_frame(liw_memory_size_type size) {
	return FrameMemBuffer::tl_frameBufferLAllocator.Allocate(size);
}

inline void liw_free_frame(void* ptr) {
	
}

template<class T, class ... Args>
inline T* liw_new_frame(Args&&... args) {
	T* ptr = (T*)liw_malloc_frame(sizeof(T));
	return new(ptr)T(std::forward<Args>(args)...);
}

template<class T>
inline void liw_delete_frame(T* ptr) {
	ptr->~T();
	liw_free_frame(ptr);
}


//
// Double frame buffer
//

const size_t SIZE_MEM_DFRAME_BUFFER = size_t{ 1 } << 26; // 64MB
const size_t SIZE_MEM_DFRAME_BUFFER_BLOCK = size_t{ 1 } << 16; // 64KB
typedef LIW::Util::LIWLGStackAllocator<SIZE_MEM_DFRAME_BUFFER, SIZE_MEM_DFRAME_BUFFER_BLOCK> DFrameBufferAllocator;
struct DFrameBuffer {
	static DFrameBufferAllocator::GlobalStackAllocator g_dframeBufferGAllocator[2];
	static thread_local DFrameBufferAllocator::LocalStackAllocator tl_dframeBufferLAllocator[2];
	static int g_dframeIdx;
};
int DFrameBuffer::g_dframeIdx{ 0 };
DFrameBufferAllocator::GlobalStackAllocator DFrameBuffer::g_dframeBufferGAllocator[2];
thread_local DFrameBufferAllocator::LocalStackAllocator DFrameBuffer::tl_dframeBufferLAllocator[2];

inline void liw_minit_dframe() {
	DFrameBuffer::g_dframeBufferGAllocator[0].Init();
	DFrameBuffer::g_dframeBufferGAllocator[1].Init();
	DFrameBuffer::g_dframeIdx = 0;
}

inline void liw_mupdate_dframe() {
	DFrameBuffer::g_dframeBufferGAllocator[DFrameBuffer::g_dframeIdx].Clear();
	DFrameBuffer::g_dframeIdx = !DFrameBuffer::g_dframeIdx;
}

inline void liw_mclnup_dframe() {
	DFrameBuffer::g_dframeBufferGAllocator[0].Cleanup();
	DFrameBuffer::g_dframeBufferGAllocator[1].Cleanup();
}

inline void liw_minit_dframe_thd() {
	DFrameBuffer::tl_dframeBufferLAllocator[0].Init(DFrameBuffer::g_dframeBufferGAllocator[0]);
	DFrameBuffer::tl_dframeBufferLAllocator[1].Init(DFrameBuffer::g_dframeBufferGAllocator[1]);
}

inline void liw_mupdate_dframe_thd() {

}

inline void liw_mclnup_dframe_thd() {

}

inline void* liw_malloc_dframe(liw_memory_size_type size) {
	return DFrameBuffer::tl_dframeBufferLAllocator[DFrameBuffer::g_dframeIdx].Allocate(size);
}

inline void liw_free_dframe(void* ptr) {

}

template<class T, class ... Args>
inline T* liw_new_dframe(Args&&... args) {
	T* ptr = (T*)liw_malloc_dframe(sizeof(T));
	return new(ptr)T(std::forward<Args>(args)...);
}

template<class T>
inline void liw_delete_dframe(T* ptr) {
	ptr->~T();
	liw_free_dframe(ptr);
}


//
// LIW memory interface
//
enum LIWMemAllocation {
	LIWMem_System,
	LIWMem_Default,
	LIWMem_Frame,
	LIWMem_DFrame,
	LIWMem_Max
};

template<LIWMemAllocation MemAlloc, class T, class ... Args>
inline T* liw_new(Args&&... args) {
	static_assert(MemAlloc < LIWMem_Max);
	switch (MemAlloc)
	{
	case LIWMem_System:
		return liw_new_sys<T>(std::forward<Args>(args)...);
	case LIWMem_Default:
		return liw_new_def<T>(std::forward<Args>(args)...);
	case LIWMem_Frame:
		return liw_new_frame<T>(std::forward<Args>(args)...);
	case LIWMem_DFrame:
		return liw_new_dframe<T>(std::forward<Args>(args)...);
	}
}

template<LIWMemAllocation MemAlloc, class T>
inline void liw_delete(T* ptr) {
	static_assert(MemAlloc < LIWMem_Max);
	switch (MemAlloc)
	{
	case LIWMem_System:
		return liw_delete_sys<T>(ptr);
	case LIWMem_Default:
		return liw_delete_def<T>(ptr);
	case LIWMem_Frame:
		return liw_delete_frame<T>(ptr);
	case LIWMem_DFrame:
		return liw_delete_dframe<T>(ptr);
	}
}