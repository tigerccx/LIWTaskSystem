#pragma once
#include <memory>
#include "LIWTypes.h"
#include "LIWLGStackAllocator.h"
#include "LIWLGGPAllocator.h"

inline const liw_memory_size_type operator""_KB(liw_memory_size_type const x) { return 1024 * x; }
inline const liw_memory_size_type operator""_MB(liw_memory_size_type const x) { return 1024 * 1024 * x; }
inline const liw_memory_size_type operator""_GB(liw_memory_size_type const x) { return 1024 * 1024 * 1024 * x; }


/*
* These are the APIs for LIW memory management subsystem. 
* LIW memory uses handles to represent its allocations. 
* 
* // Memory subsystem maintenance functions
* liw_minit_MODE:			initialize (once per run)
* liw_mupdate_MODE:			update (once per post frame)
* liw_mclnup_MODE:			cleanup (once per run)
* liw_minit_MODE_thd:		initialize (once per run per thread)
* liw_mupdate_MODE_thd:		update (once per post frame per thread)
* liw_mclnup_MODE_thd:		cleanup (once per run per thread)
* 
* // Memory access functions
* liw_maddr_MODE:			get raw address
* liw_mset_MODE:			set value
* liw_mget_MODE:			get value
* liw_malloc_MODE:			malloc
* liw_free_MODE:			free
* liw_new_MODE:				new
* liw_delete_MODE:			delete
*/


//
// System memory allocation
// 

inline void* liw_maddr_sys(liw_hdl_type handle) {
	return (void*)handle;
}

template<class T>
inline void liw_mset_sys(liw_hdl_type handle, const T& val) {
	*((T*)handle) = val;
}
template<class T>
inline void liw_mset_sys(liw_hdl_type handle, T&& val) {
	*((T*)handle) = val;
}

template<class T>
inline T liw_mget_sys(liw_hdl_type handle) {
	return std::move(*((T*)handle));
}

inline liw_hdl_type liw_malloc_sys(liw_memory_size_type size) {
	return (liw_hdl_type)malloc(size);
}

inline void liw_free_sys(liw_hdl_type handle) {
	free((void*)handle);
}

template<class T, class ... Args>
inline liw_hdl_type liw_new_sys(Args&&... args) {
	T* ptr = (T*)liw_malloc_sys(sizeof(T));
	return (liw_hdl_type)(new(ptr)T(std::forward<Args>(args)...));
}

template<class T>
inline void liw_delete_sys(liw_hdl_type handle) {
	((T*)handle)->~T();
	liw_free_sys(handle);
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

inline void* liw_maddr_def(liw_hdl_type handle) {
	return DefaultBufferAllocator::GetAddressFromHandle(handle);
}

template<class T>
inline void liw_mset_def(liw_hdl_type handle, const T& val) {
	DefaultBufferAllocator::SetMem<T>(handle, std::forward<T>(val));
}
template<class T>
inline void liw_mset_def(liw_hdl_type handle, T&& val) {
	DefaultBufferAllocator::SetMem<T>(handle, std::forward<T>(val));
}

template<class T>
inline T liw_mget_def(liw_hdl_type handle) {
	return std::move(DefaultBufferAllocator::GetMem<T>(handle));
}

inline liw_hdl_type liw_malloc_def(liw_memory_size_type size) {
	return DefaultMemBuffer::tl_defaultBufferLAllocator.Allocate(size);
}

inline void liw_free_def(liw_hdl_type handle) {
	DefaultMemBuffer::tl_defaultBufferLAllocator.Free(handle);
}

template<class T, class ... Args>
inline liw_hdl_type liw_new_def(Args&&... args) {
	liw_hdl_type handle = liw_malloc_def(sizeof(T));
	T* ptr = (T*)liw_maddr_def(handle);
	new(ptr)T(std::forward<Args>(args)...);
	return handle;
}

template<class T>
inline void liw_delete_def(liw_hdl_type handle) {
	T* ptr = (T*)liw_maddr_def(handle);
	ptr->~T();
	liw_free_def(handle);
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

inline void* liw_maddr_static(liw_hdl_type handle) {
	return (void*)handle;
}

template<class T>
inline void liw_mset_static(liw_hdl_type handle, const T& val) {
	*((T*)handle) = val;
}
template<class T>
inline void liw_mset_static(liw_hdl_type handle, T&& val) {
	*((T*)handle) = val;
}

template<class T>
inline T liw_mget_static(liw_hdl_type handle) {
	return std::move(*((T*)handle));
}

inline liw_hdl_type liw_malloc_static(liw_memory_size_type size) {
	return (liw_hdl_type)StaticMemBuffer::tl_staticBufferLAllocator.Allocate(size);
}

inline void liw_free_static(liw_hdl_type handle) {

}

template<class T, class ... Args>
inline liw_hdl_type liw_new_static(Args&&... args) {
	T* ptr = (T*)liw_malloc_static(sizeof(T));
	return (liw_hdl_type)(new(ptr)T(std::forward<Args>(args)...));
}

template<class T>
inline void liw_delete_static(liw_hdl_type handle) {
	((T*)handle)->~T();
	liw_free_static(handle);
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

inline void* liw_maddr_frame(liw_hdl_type handle) {
	return (void*)handle;
}

template<class T>
inline void liw_mset_frame(liw_hdl_type handle, const T& val) {
	*((T*)handle) = val;
}
template<class T>
inline void liw_mset_frame(liw_hdl_type handle, T&& val) {
	*((T*)handle) = val;
}

template<class T>
inline T liw_mget_frame(liw_hdl_type handle) {
	return std::move(*((T*)handle));
}

inline liw_hdl_type liw_malloc_frame(liw_memory_size_type size) {
	return (liw_hdl_type)FrameMemBuffer::tl_frameBufferLAllocator.Allocate(size);
}

inline void liw_free_frame(liw_hdl_type handle) {
	
}

template<class T, class ... Args>
inline liw_hdl_type liw_new_frame(Args&&... args) {
	T* ptr = (T*)liw_malloc_frame(sizeof(T));
	return (liw_hdl_type)(new(ptr)T(std::forward<Args>(args)...));
}

template<class T>
inline void liw_delete_frame(liw_hdl_type handle) {
	((T*)handle)->~T();
	liw_free_frame(handle);
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

inline void* liw_maddr_dframe(liw_hdl_type handle) {
	return (void*)handle;
}

template<class T>
inline void liw_mset_dframe(liw_hdl_type handle, const T& val) {
	*((T*)handle) = val;
}
template<class T>
inline void liw_mset_dframe(liw_hdl_type handle, T&& val) {
	*((T*)handle) = val;
}

template<class T>
inline T liw_mget_dframe(liw_hdl_type handle) {
	return std::move(*((T*)handle));
}

inline liw_hdl_type liw_malloc_dframe(liw_memory_size_type size) {
	return (liw_hdl_type)DFrameBuffer::tl_dframeBufferLAllocator[DFrameBuffer::g_dframeIdx].Allocate(size);
}

inline void liw_free_dframe(liw_hdl_type handle) {

}

template<class T, class ... Args>
inline liw_hdl_type liw_new_dframe(Args&&... args) {
	T* ptr = (T*)liw_malloc_dframe(sizeof(T));
	return (liw_hdl_type)(new(ptr)T(std::forward<Args>(args)...));
}

template<class T>
inline void liw_delete_dframe(liw_hdl_type handle) {
	((T*)handle)->~T();
	liw_free_dframe(handle);
}


//
// LIW memory interface
//
enum LIWMemAllocation {
	LIWMem_System,
	LIWMem_Default,
	LIWMem_Static,
	LIWMem_Frame,
	LIWMem_DFrame,
	LIWMem_Max
};


template<LIWMemAllocation MemAlloc>
inline void* liw_maddr(liw_hdl_type handle) {
	static_assert(MemAlloc < LIWMem_Max, "Must use a valid LIWMemAllocation enum. ");
	switch (MemAlloc)
	{
	case LIWMem_System:
		return liw_maddr_sys(handle);
	case LIWMem_Default:
		return liw_maddr_def(handle);
	case LIWMem_Static:
		return liw_maddr_static(handle);
	case LIWMem_Frame:
		return liw_maddr_frame(handle);
	case LIWMem_DFrame:
		return liw_maddr_dframe(handle);
	}
}

template<LIWMemAllocation MemAlloc, class T>
inline void liw_mset(liw_hdl_type handle, T&& val) {
	static_assert(MemAlloc < LIWMem_Max, "Must use a valid LIWMemAllocation enum. ");
	switch (MemAlloc)
	{
	case LIWMem_System:
		liw_mset_sys<T>(handle, std::forward<T>(val)); break;
	case LIWMem_Default:
		liw_mset_def<T>(handle, std::forward<T>(val)); break;
	case LIWMem_Static:
		liw_mset_static<T>(handle, std::forward<T>(val)); break;
	case LIWMem_Frame:
		liw_mset_frame<T>(handle, std::forward<T>(val)); break;
	case LIWMem_DFrame:
		liw_mset_dframe<T>(handle, std::forward<T>(val)); break;
	}
}
template<LIWMemAllocation MemAlloc, class T>
inline void liw_mset(liw_hdl_type handle, const T& val) {
	static_assert(MemAlloc < LIWMem_Max, "Must use a valid LIWMemAllocation enum. ");
	switch (MemAlloc)
	{
	case LIWMem_System:
		liw_mset_sys<T>(handle, std::forward<T>(val)); break;
	case LIWMem_Default:
		liw_mset_def<T>(handle, std::forward<T>(val)); break;
	case LIWMem_Static:
		liw_mset_static<T>(handle, std::forward<T>(val)); break;
	case LIWMem_Frame:
		liw_mset_frame<T>(handle, std::forward<T>(val)); break;
	case LIWMem_DFrame:
		liw_mset_dframe<T>(handle, std::forward<T>(val)); break;
	}
}

template<LIWMemAllocation MemAlloc, class T>
inline T liw_mget(liw_hdl_type handle) {
	static_assert(MemAlloc < LIWMem_Max, "Must use a valid LIWMemAllocation enum. ");
	switch (MemAlloc)
	{
	case LIWMem_System:
		return std::move(liw_mget_sys<T>(handle)); 
	case LIWMem_Default:
		return std::move(liw_mget_def<T>(handle));
	case LIWMem_Static:
		return std::move(liw_mget_static<T>(handle));
	case LIWMem_Frame:
		return std::move(liw_mget_frame<T>(handle));
	case LIWMem_DFrame:
		return std::move(liw_mget_dframe<T>(handle));
	}
}


template<LIWMemAllocation MemAlloc, class T, class ... Args>
inline liw_hdl_type liw_new(Args&&... args) {
	static_assert(MemAlloc < LIWMem_Max, "Must use a valid LIWMemAllocation enum. ");
	switch (MemAlloc)
	{
	case LIWMem_System:
		return liw_new_sys<T>(std::forward<Args>(args)...);
	case LIWMem_Default:
		return liw_new_def<T>(std::forward<Args>(args)...);
	case LIWMem_Static:
		return liw_new_static<T>(std::forward<Args>(args)...);
	case LIWMem_Frame:
		return liw_new_frame<T>(std::forward<Args>(args)...);
	case LIWMem_DFrame:
		return liw_new_dframe<T>(std::forward<Args>(args)...);
	}
}

template<LIWMemAllocation MemAlloc, class T>
inline void liw_delete(liw_hdl_type handle) {
	static_assert(MemAlloc < LIWMem_Max, "Must use a valid LIWMemAllocation enum. ");
	switch (MemAlloc)
	{
	case LIWMem_System:
		liw_delete_sys<T>(handle); break;
	case LIWMem_Default:
		liw_delete_def<T>(handle); break;
	case LIWMem_Static:
		liw_delete_static<T>(handle); break;
	case LIWMem_Frame:
		liw_delete_frame<T>(handle); break;
	case LIWMem_DFrame:
		liw_delete_dframe<T>(handle); break;
	}
}