#include "StringTextureCache.h"

void StringTextureCache::Clear() noexcept
{
	Cache.clear();
}

bool StringTextureCache::Find(HFONT fontHandle, const wchar_t* string, TextureInfo& info)
{
	std::map<std::pair<HANDLE, std::wstring>, TextureInfo>::iterator it = Cache.find(std::make_pair(fontHandle, string));

	if (it != Cache.end())
	{
		info = it->second;

		return true;
	}

	return false;
}

void StringTextureCache::Insert(HFONT fontHandle, const wchar_t* string, const TextureInfo& info)
{
	Cache.insert({ std::make_pair(fontHandle, string), info });
}

bool StringTextureCache::CacheComparer::operator()(const std::pair<HANDLE, std::wstring_view>& a, const std::pair<HANDLE, std::wstring_view>& b) const
{
	if (a.first != b.first)
		return a.first < b.first;

	int stringDifference = a.second.compare(b.second);

	if (stringDifference != 0)
		return stringDifference < 0;

	return false;
}
