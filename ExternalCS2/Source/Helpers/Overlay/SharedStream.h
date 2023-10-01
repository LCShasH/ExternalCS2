#pragma once

#include <memory>
#include <Windows.h>

#include "CommonDeleters.h"

class SharedStream
{
public:
	SharedStream(const wchar_t* nameFormatFormat, ...);

	bool Get(void* dst, DWORD size) const noexcept;

	bool PutFront(int value, DWORD count) noexcept;
	bool PutFront(const void* src, DWORD size) noexcept;

	bool PutBack(int value, DWORD count) noexcept;
	bool PutBack(const void* src, DWORD size) noexcept;

	HANDLE GetWrittenHandle() const noexcept;
	HANDLE GetAvailableHandle() const noexcept;
	HANDLE GetMutexHandle() const noexcept;

	DWORD GetSize() const noexcept;
	DWORD GetCapacity() const noexcept;
	DWORD GetUnusedCapacity() const noexcept;

private:
	std::unique_ptr<HANDLE, HandleDeleter> WrittenHandle;
	std::unique_ptr<HANDLE, HandleDeleter> AvailableHandle;
	std::unique_ptr<HANDLE, HandleDeleter> MutexHandle;
	std::unique_ptr<HANDLE, HandleDeleter> FileMappingHandle;
	std::unique_ptr<LPVOID, MemoryMappedViewDeleter> FileMappingView;

	struct
	{
		DWORD StartIndex;
		DWORD EndIndex;
		DWORD Capacity;
		DWORD Size;
	} *Header;

	char* Buffer;
};