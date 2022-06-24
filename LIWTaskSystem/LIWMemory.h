#pragma once
#include <memory>

typedef size_t liw_memory_size_type;

//TODO: Build memory allocation mechanism
inline void* liw_malloc(liw_memory_size_type size) {
	return malloc(size);
}

inline void liw_free(void* ptr) {
	free(ptr);
}

template<class T, class ... Args>
inline T* liw_new(Args&&... args) {
	T* ptr = (T*)liw_malloc(sizeof(T));
	return new(ptr)T(std::forward<Args>(args)...);
}

template<class T>
inline void liw_delete(T* ptr) {
	ptr->~T();
	liw_free(ptr);
}

