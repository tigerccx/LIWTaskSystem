#pragma once
#include <memory>
#include <cassert>

/// <summary>
/// align the address with alignment
/// </summary>
/// <param name="address"> address to align </param>
/// <param name="align"> alignment </param>
/// <returns> aligned address </returns>
inline uintptr_t liw_align_address(uintptr_t address, size_t align) {
	const uintptr_t mask = align - 1;
	assert((align & mask) == 0);
	return (address + mask) & (~mask);
}

/// <summary>
/// align the pointer with alignment
/// </summary>
/// <typeparam name="T"> type of pointer </typeparam>
/// <param name="ptr"> pointer to align </param>
/// <param name="align"> alignment </param>
/// <returns> aligned pointer </returns>
template<class T>
inline T* liw_align_pointer(T* ptr, size_t align) {
	uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
	address = liw_align_address(address, align);
	return reinterpret_cast<T*>(address);
}