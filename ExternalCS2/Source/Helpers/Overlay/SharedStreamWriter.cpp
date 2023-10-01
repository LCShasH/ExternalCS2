#include "SharedStreamWriter.h"

SharedStreamWriter::SharedStreamWriter(SharedStream& stream, bool commitFront) :
	Stream(stream),
	Capacity(stream.GetCapacity()),
	Buffer(new char[Capacity]),
	CommitFront(commitFront)
{
}

void SharedStreamWriter::Clear() noexcept
{
	Size = 0;
}

bool SharedStreamWriter::Commit() noexcept
{
	return CommitFront
		? Stream.PutFront(Buffer.get(), Size)
		: Stream.PutBack(Buffer.get(), Size);
}

bool SharedStreamWriter::Put(int value, DWORD count) noexcept
{
	if (count > GetUnusedCapacity())
		return false;

	memset(Buffer.get() + Size, value, count);
	Size += count;

	return true;
}

bool SharedStreamWriter::Put(const void* src, DWORD size) noexcept
{
	if (size > GetUnusedCapacity())
		return false;

	memcpy(Buffer.get() + Size, src, size);
	Size += size;

	return true;
}

DWORD SharedStreamWriter::GetSize() const noexcept
{
	return Size;
}

DWORD SharedStreamWriter::GetCapacity() const noexcept
{
	return Capacity;
}

DWORD SharedStreamWriter::GetUnusedCapacity() const noexcept
{
	return Size < Capacity
		? Capacity - Size
		: 0;
}
