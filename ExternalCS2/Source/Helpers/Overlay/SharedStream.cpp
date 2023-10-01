#include <stdexcept>

#include "SharedStream.h"

SharedStream::SharedStream(const wchar_t* nameFormatFormat, ...)
{
	va_list args;

	va_start(args, nameFormatFormat);

	wchar_t nameFormat[MAX_PATH];
	int nameFormatWritten = vswprintf(nameFormat, ARRAYSIZE(nameFormat), nameFormatFormat, args);

	va_end(args);

	wchar_t name[MAX_PATH];

	// error checking the longest name only, the rest will format if this does
	if (nameFormatWritten < 0 || swprintf(name, ARRAYSIZE(name), nameFormat, L"written") < 0)
		throw std::invalid_argument("Name format format is invalid.");

	HANDLE writtenHandle = OpenEventW(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, name);

	if (writtenHandle == nullptr)
		throw std::runtime_error("Failed to open a handle to the written event object.");

	WrittenHandle.reset(writtenHandle);

	swprintf(name, ARRAYSIZE(name), nameFormat, L"avail");
	HANDLE availableHandle = OpenEventW(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, name);

	if (availableHandle == nullptr)
		throw std::runtime_error("Failed to open a handle to the available event object.");

	AvailableHandle.reset(availableHandle);

	swprintf(name, ARRAYSIZE(name), nameFormat, L"mutex");
	HANDLE mutexHandle = OpenMutexW(SYNCHRONIZE, FALSE, name);

	if (mutexHandle == nullptr)
		throw std::runtime_error("Failed to open a handle to the mutex object.");

	MutexHandle.reset(mutexHandle);

	swprintf(name, MAX_PATH, nameFormat, L"mem");
	HANDLE fileMappingHandle = OpenFileMappingW(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, name);

	if (fileMappingHandle == nullptr)
		throw std::runtime_error("Failed to open a handle to the file mapping object.");

	FileMappingHandle.reset(fileMappingHandle);

	LPVOID fileMappingView = MapViewOfFile(fileMappingHandle, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);

	if (fileMappingView == nullptr)
		throw std::runtime_error("Failed to map a view of the file mapping object.");

	FileMappingView.reset(fileMappingView);

	Header = (decltype(Header))fileMappingView;
	Buffer = (char*)fileMappingView + sizeof(*Header);
}

bool SharedStream::Get(void* dst, DWORD size) const noexcept
{
	if (size > Header->Size)
		return false;

	char* src = Buffer + Header->StartIndex;

	// stream is wrapped
	if (Header->StartIndex > Header->EndIndex)
	{
		DWORD prewrapSize = Header->Capacity - Header->StartIndex;

		// get is wrapped
		if (size > prewrapSize)
		{
			memcpy(dst, src, prewrapSize);
			// update args
			dst = (char*)dst + prewrapSize;
			size -= prewrapSize;
			// wrap source
			src = Buffer;
		}
	}

	memcpy(dst, src, size);

	return true;
}

bool SharedStream::PutFront(int value, DWORD count) noexcept
{
	if (count > GetUnusedCapacity())
		return false;

	char* dst;

	Header->Size += count;

	// write is wrapped
	if (count > Header->StartIndex)
	{
		DWORD postwrapSize = count - Header->StartIndex;
		// update count to write for the common set
		count = Header->StartIndex;
		// wrap buffer start index
		Header->StartIndex = Header->Capacity - postwrapSize;

		memset(Buffer + Header->StartIndex, value, postwrapSize);
		// finish write at start
		dst = Buffer;
	}

	else
	{
		Header->StartIndex -= count;
		dst = Buffer + Header->StartIndex;
	}

	memset(dst, value, count);

	return true;
}

bool SharedStream::PutFront(const void* src, DWORD size) noexcept
{
	if (size > GetUnusedCapacity())
		return false;

	char* dst;

	Header->Size += size;

	// write is wrapped
	if (size > Header->StartIndex)
	{
		DWORD postwrapSize = size - Header->StartIndex;
		// update size to write for the common copy
		size = Header->StartIndex;
		// wrap buffer start index
		Header->StartIndex = Header->Capacity - postwrapSize;

		memcpy(Buffer + Header->StartIndex, src, postwrapSize);
		// update source
		src = (char*)src + postwrapSize;
		// finish write at start
		dst = Buffer;
	}

	else
	{
		Header->StartIndex -= size;
		dst = Buffer + Header->StartIndex;
	}

	memcpy(dst, src, size);

	return true;
}

bool SharedStream::PutBack(int value, DWORD count) noexcept
{
	if (count > GetUnusedCapacity())
		return false;

	char* dst;

	Header->Size += count;
	DWORD prewrapSize = Header->Capacity - Header->EndIndex;

	// write is wrapped
	if (count > prewrapSize)
	{
		DWORD postwrapSize = count - prewrapSize;
		// update size to write for the common copy
		count = postwrapSize;

		memset(Buffer + Header->EndIndex, value, prewrapSize);
		// set the new (wrapped) end index
		Header->EndIndex = postwrapSize;
		// finish write at start
		dst = Buffer;
	}

	else
	{
		Header->EndIndex += count;
		dst = Buffer + Header->EndIndex;
	}

	memset(dst, value, count);

	return true;
}

bool SharedStream::PutBack(const void* src, DWORD size) noexcept
{
	if (size > GetUnusedCapacity())
		return false;

	char* dst;

	Header->Size += size;
	DWORD prewrapSize = Header->Capacity - Header->EndIndex;

	// write is wrapped
	if (size > prewrapSize)
	{
		DWORD postwrapSize = size - prewrapSize;
		// update size to write for the common copy
		size = postwrapSize;

		memcpy(Buffer + Header->EndIndex, src, prewrapSize);
		// set the new (wrapped) end index
		Header->EndIndex = postwrapSize;
		// update source
		src = (char*)src + prewrapSize;
		// finish write at start
		dst = Buffer;
	}

	else
	{
		Header->EndIndex += size;
		dst = Buffer + Header->EndIndex;
	}

	memcpy(dst, src, size);

	return true;
}

HANDLE SharedStream::GetWrittenHandle() const noexcept
{
	return WrittenHandle.get();
}

HANDLE SharedStream::GetAvailableHandle() const noexcept
{
	return AvailableHandle.get();
}

HANDLE SharedStream::GetMutexHandle() const noexcept
{
	return MutexHandle.get();
}

DWORD SharedStream::GetSize() const noexcept
{
	return Header->Size;
}

DWORD SharedStream::GetCapacity() const noexcept
{
	return Header->Capacity;
}

DWORD SharedStream::GetUnusedCapacity() const noexcept
{
	return Header->Size < Header->Capacity
		? Header->Capacity - Header->Size
		: 0;
}
