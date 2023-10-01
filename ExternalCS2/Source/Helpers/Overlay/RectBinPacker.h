#pragma once

#include <vector>
#include <Windows.h>

class RectBinPacker
{
public:
	RectBinPacker(LONG width, LONG height);

	void Reset();
	bool Pack(RECT& rect);

	SIZE GetSize() const noexcept;

private:
	SIZE Size;
	std::vector<RECT> FreeRects;
};