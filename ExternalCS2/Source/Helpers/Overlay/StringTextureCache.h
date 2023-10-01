#pragma once

#include <map>
#include <string>
#include <Windows.h>
#include <string_view>

#include "TextureInfo.h"

class StringTextureCache
{
public:
	void Clear() noexcept;
	bool Find(HFONT fontHandle, const wchar_t* string, TextureInfo& info);
	void Insert(HFONT fontHandle, const wchar_t* string, const TextureInfo& info);

private:
	struct CacheComparer
	{
		using is_transparent = void;

		bool operator()(const std::pair<HANDLE, std::wstring_view>& a, const std::pair<HANDLE, std::wstring_view>& b) const;
	};

	std::map<std::pair<HANDLE, std::wstring>, TextureInfo, CacheComparer> Cache;
};