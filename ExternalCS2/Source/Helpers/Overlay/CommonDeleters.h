#pragma once

#include <Windows.h>

struct HandleDeleter
{
	typedef HANDLE pointer;

	void operator()(HANDLE handle) noexcept
	{
		CloseHandle(handle);
	}
};

struct MemoryMappedViewDeleter
{
	typedef LPVOID pointer;

	void operator()(LPVOID view) noexcept
	{
		UnmapViewOfFile(view);
	}
};

struct DcDeleter
{
	typedef HDC pointer;

	void operator()(HDC dcHandle) noexcept
	{
		DeleteDC(dcHandle);
	}
};