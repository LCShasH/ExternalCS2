#pragma once

#include <memory>
#include <Windows.h>

#include "SharedStream.h"

class SharedStreamWriter
{
public:
	SharedStreamWriter(SharedStream& stream, bool commitFront = false);

	void Clear() noexcept;
	bool Commit() noexcept;

	bool Put(int value, DWORD count) noexcept;
	bool Put(const void* src, DWORD size) noexcept;

	DWORD GetSize() const noexcept;
	DWORD GetCapacity() const noexcept;
	DWORD GetUnusedCapacity() const noexcept;

private:
	SharedStream& Stream;
	DWORD Capacity;
	std::unique_ptr<char[]> Buffer;
	bool CommitFront;
	DWORD Size = 0;
};