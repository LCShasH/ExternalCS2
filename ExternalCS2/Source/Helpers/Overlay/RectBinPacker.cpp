#include "RectBinPacker.h"

// GDI helper function
static SIZE GetRectSize(const RECT& rect)
{
	SIZE ret =
	{
		rect.right - rect.left,
		rect.bottom - rect.top
	};

	return ret;
}

RectBinPacker::RectBinPacker(LONG width, LONG height) :
	Size({ width, height })
{
	Reset();
}

void RectBinPacker::Reset()
{
	FreeRects.clear();
	FreeRects.push_back({ 0, 0, Size.cx, Size.cy });
}

bool RectBinPacker::Pack(RECT& rect)
{
	SIZE rectSize = GetRectSize(rect);

	std::vector<RECT>::iterator best;
	LONG bestRemainingWidth = LONG_MAX;
	LONG bestRemainingHeight = LONG_MAX;

	for (std::vector<RECT>::iterator freeRect = FreeRects.begin();
		freeRect != FreeRects.end();
		++freeRect)
	{
		SIZE freeRectSize = GetRectSize(*freeRect);

		// rect fits
		if (rectSize.cx <= freeRectSize.cx && rectSize.cy <= freeRectSize.cy)
		{
			LONG remainingWidth = freeRectSize.cx - rectSize.cx;
			LONG remainingHeight = freeRectSize.cy - rectSize.cy;

			if (remainingWidth < bestRemainingWidth || (remainingWidth == bestRemainingWidth && remainingHeight < bestRemainingHeight))
			{
				best = freeRect;
				bestRemainingWidth = remainingWidth;
				bestRemainingHeight = remainingHeight;
			}
		}
	}

	// no fit found
	if (bestRemainingWidth == LONG_MAX)
		return false;

	// update rect values
	rect.left = best->left;
	rect.top = best->top;
	rect.right = rect.left + rectSize.cx;
	rect.bottom = rect.top + rectSize.cy;

	// exact fit, remove
	if (bestRemainingWidth == 0 && bestRemainingHeight == 0)
	{
		FreeRects.erase(best);
	}

	// space left over, split
	else
	{
		bool splitByWidth = bestRemainingWidth > bestRemainingHeight;

		// x
		if (splitByWidth)
			best->left = rect.right;

		// y
		else
			best->top = rect.bottom;

		// 2nd split
		if (bestRemainingWidth != 0 && bestRemainingHeight != 0)
		{
			RECT newFreeRect;

			// y
			if (splitByWidth)
			{
				newFreeRect.left = rect.left;
				newFreeRect.top = rect.bottom;
				newFreeRect.right = rect.right;
				newFreeRect.bottom = best->bottom;
			}

			// x 
			else
			{
				newFreeRect.left = rect.right;
				newFreeRect.top = rect.top;
				newFreeRect.right = best->right;
				newFreeRect.bottom = best->top;
			}

			FreeRects.push_back(newFreeRect);
		}
	}

	return true;
}

SIZE RectBinPacker::GetSize() const noexcept
{
	return Size;
}
