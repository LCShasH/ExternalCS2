#pragma once

#include <map>
#include <atomic>
#include <thread>
#include <memory>
#include <Windows.h>

#include "SharedStream.h"
#include "RectBinPacker.h"
#include "SharedStreamWriter.h"
#include "StringTextureCache.h"

enum class GradientDirection : DWORD
{
	Horizontal = 1,
	Vertical = 2,
	None = 3
};

class TexturePage : public RectBinPacker
{
public:
	TexturePage(long textureId, LONG width, LONG height);

	long GetTextureId() const noexcept;

private:
	long TextureId;
};

class Overlay
{
public:
	Overlay(DWORD processId, DWORD waitTimeout = 1000);
	~Overlay();

	bool BeginFrame() noexcept;
	void EndFrame() noexcept;

	void DrawString(long x, long y, HFONT fontHandle, const wchar_t* string);
	void DrawFilledRect(long x0, long y0, long x1, long y1, DWORD color);
	void DrawRect(long x0, long y0, long x1, long y1, long thikness, DWORD color);

	bool DeleteTexture(long textureId) noexcept;

	bool LoadTexture(long textureId, long fullUpdate, DWORD size, DWORD width, DWORD height, long x, long y);
	bool LoadTexture(long textureId, long fullUpdate, DWORD size, DWORD width, DWORD height, long x, long y, const void* rgba);

	void DrawTexturedRect(long x0, long y0, long x1, long y1, float u0, float v0, float u1, float v1, DWORD colorStart, DWORD colorEnd, GradientDirection gradient, long textureId);

	long GetNewTextureId() noexcept;

private:
	void InputThreadRoutine();

	TexturePage CreateAndLoadNewTexturePage(LONG width, LONG height);

	bool LoadTextureInternal(long textureId, long fullUpdate, DWORD size, DWORD width, DWORD height, long x, long y);

	SharedStream InputStream;
	SharedStream RenderStream;
	DWORD WaitTimeout;
	std::unique_ptr<HDC, DcDeleter> DcHandle;

	long TextureIdCounter = 0;
	std::map<long, long> TextureVersions;
	SharedStreamWriter RenderBuffer;
	TexturePage StringPage;
	StringTextureCache StringCache;

	std::atomic<bool> IsClosing = false;
	bool AckNeeded;
	long AckTextureId;
	long AckVersion;
	std::thread InputThread;
};