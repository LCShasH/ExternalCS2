#define _USE_MATH_DEFINES
#include <math.h>
#include <stdexcept>

#include "Overlay.h"

enum class InputEvent : DWORD
{
	EnableOverlay = 0,
	DisableOverlay = 1,
	SetGeneralInformation = 2, // name?
	SetFpsBannerTextureId = 3, // name?
	RenderedFrame = 5, // name?
	TextureReceived = 6, // name?
	SetGameId = 7,
	SetNotificationPosition = 8,
	TextureDeleted = 9,
	DeletedAllTextures = 10, // name?
	Unknown = 11, // name?
	SetImeLanguageList = 12,
	TakeScreenshot = 13, // name?
	Log = 14,
	VrModeBegin = 15,
	VrModeEnd = 16,
	SetIngameFps = 17,
	SetScreenEdgePadding = 18,
	SerializeAllTextures = 19,
	UpdateVrState = 20
};

enum class RenderCommand : DWORD
{
	BeginFrame = 0,
	LoadTexture = 1,
	DrawTexturedRect = 3,
	EndFrame = 4,
	DeleteTexture = 11
};

TexturePage::TexturePage(long textureId, LONG width, LONG height) :
	RectBinPacker(width, height),
	TextureId(textureId)
{
}

long TexturePage::GetTextureId() const noexcept
{
	return TextureId;
}

Overlay::Overlay(DWORD processId, DWORD waitTimeout) :
	InputStream(L"GameOverlay_InputEventStream_%u_%%ls-IPCWrapper", processId),
	RenderStream(L"GameOverlayRender_PaintCmdStream_%u_%%ls-IPCWrapper", processId),
	WaitTimeout(waitTimeout),
	RenderBuffer(RenderStream, true),
	StringPage(CreateAndLoadNewTexturePage(1024, 512))
{
	HDC dcHandle = CreateCompatibleDC(nullptr);

	if (dcHandle == nullptr)
		throw std::runtime_error("Failed to create a new memory device context.");

	DcHandle.reset(dcHandle);

	// needs to go last for safety
	InputThread = std::thread(&Overlay::InputThreadRoutine, this);
}

Overlay::~Overlay()
{
	IsClosing = true;
	InputThread.join();
}

bool Overlay::BeginFrame() noexcept
{
	HANDLE handles[] =
	{
		InputStream.GetMutexHandle(),
		RenderStream.GetMutexHandle(),
		InputStream.GetAvailableHandle()
	};

	if (WaitForMultipleObjects(ARRAYSIZE(handles), handles, TRUE, WaitTimeout) != WAIT_OBJECT_0)
		return false;

	// frame not ack'd yet
	if (AckNeeded)
	{
		EndFrame();

		return false;
	}

	RenderBuffer.Clear();

	return true;
}

void Overlay::EndFrame() noexcept
{
	RenderBuffer.Commit();

	ReleaseMutex(InputStream.GetMutexHandle());
	ReleaseMutex(RenderStream.GetMutexHandle());
}

void Overlay::DrawString(long x, long y, HFONT fontHandle, const wchar_t* string)
{
	TextureInfo info;

	if (!StringCache.Find(fontHandle, string, info))
	{
		SelectObject(DcHandle.get(), fontHandle);

		// calculate string info
		size_t stringLength = wcslen(string);
		RECT stringRect = {};

		DrawTextW(DcHandle.get(), string, stringLength, &stringRect, DT_CALCRECT);

		// clamp string size
		SIZE pageSize = StringPage.GetSize();

		if (stringRect.right > pageSize.cx)
			stringRect.right = pageSize.cx;

		if (stringRect.bottom > pageSize.cy)
			stringRect.bottom = pageSize.cy;

		// pack into page
		RECT packedStringRect = stringRect;

		if (!StringPage.Pack(packedStringRect))
		{
			StringPage.Reset();
			StringCache.Clear();

			StringPage.Pack(packedStringRect); // always successful
		}

		DWORD size = stringRect.right * stringRect.bottom * 4;

		// setup bitmap
		BITMAPINFOHEADER bmpInfoHeader = {};
		bmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmpInfoHeader.biWidth = stringRect.right;
		bmpInfoHeader.biHeight = -stringRect.bottom;
		bmpInfoHeader.biSizeImage = size;
		bmpInfoHeader.biPlanes = 1;
		bmpInfoHeader.biBitCount = 32;
		bmpInfoHeader.biCompression = BI_RGB;

		char* buffer;
		HBITMAP dibHandle = CreateDIBSection(DcHandle.get(), (BITMAPINFO*)&bmpInfoHeader, DIB_RGB_COLORS, (void**)&buffer, nullptr, 0);

		SelectObject(DcHandle.get(), dibHandle);

		// draw the string
		SetBkMode(DcHandle.get(), TRANSPARENT);
		SetTextColor(DcHandle.get(), RGB(255, 255, 255));
		DrawTextW(DcHandle.get(), string, stringLength, &stringRect, DT_NOCLIP | DT_END_ELLIPSIS);

		// correct alpha
		for (size_t i = 0; i < size; i += 4)
		{
			char r = buffer[i];
			char g = buffer[i + 1];
			char b = buffer[i + 2];
			buffer[i + 3] = (char)((float)r * 0.34f + (float)g * 0.55f + (float)b * 0.11f);
		}

		// try to load the texture
		long textureId = StringPage.GetTextureId();
		bool loaded = LoadTexture(textureId, 0, size, stringRect.right, stringRect.bottom, packedStringRect.left, packedStringRect.top, buffer);

		DeleteObject(dibHandle);

		// not enough room in the render buffer
		// maybe come up with a way of doing this before expensive rendering?
		if (!loaded)
			return;

		info.TextureId = textureId;

		info.Coordinates[0] = (float)packedStringRect.left / pageSize.cx;
		info.Coordinates[1] = (float)packedStringRect.top / pageSize.cy;
		info.Coordinates[2] = (float)packedStringRect.right / pageSize.cx;
		info.Coordinates[3] = (float)packedStringRect.bottom / pageSize.cy;

		info.Size =
		{
			stringRect.right,
			stringRect.bottom
		};

		StringCache.Insert(fontHandle, string, info);
	}

	DrawTexturedRect(x, y,
		x + info.Size.cx, y + info.Size.cy,
		info.Coordinates[0], info.Coordinates[1], info.Coordinates[2], info.Coordinates[3],
		0xFFFFFFFF, 0x0,
		GradientDirection::None,
		info.TextureId);
}

void Overlay::DrawFilledRect(long x0, long y0, long x1, long y1, DWORD color)
{
	static auto texture = CreateAndLoadNewTexturePage(1, 1);
	DrawTexturedRect(x0, y0, x1, y1, 0.0f, 0.0f, 1.0f, 1.0f, color, color, GradientDirection::None, texture.GetTextureId());
}

void Overlay::DrawRect(long x0, long y0, long x1, long y1, long thikness, DWORD color)
{
	auto width = x1 - x0;
	auto height = y1 - y0;

	// left
	DrawFilledRect(x0, y0, x0 + thikness, y0 + height, color);

	// down
	DrawFilledRect(x0 + thikness, y0 + height - thikness, x1, y1, color);

	// right
	DrawFilledRect(x1, y1 - height, x1 + thikness, y1, color);

	// top
	DrawFilledRect(x0, y0, x0 + width, y0 + thikness, color);
}

bool Overlay::LoadTexture(long textureId, long fullUpdate, DWORD size, DWORD width, DWORD height, long x, long y)
{
	if (!LoadTextureInternal(textureId, fullUpdate, size, width, height, x, y))
		return false;

	RenderBuffer.Put(0xBF, size);

	return true;
}

bool Overlay::LoadTexture(long textureId, long fullUpdate, DWORD size, DWORD width, DWORD height, long x, long y, const void* rgba)
{
	if (!LoadTextureInternal(textureId, fullUpdate, size, width, height, x, y))
		return false;

	RenderBuffer.Put(rgba, size);

	return true;
}

void Overlay::DrawTexturedRect(long x0, long y0, long x1, long y1, float u0, float v0, float u1, float v1, DWORD colorStart, DWORD colorEnd, GradientDirection gradient, long textureId)
{
	struct
	{
		RenderCommand Command;
		long X0;
		long Y0;
		long X1;
		long Y1;
		float U0;
		float V0;
		float U1;
		float V1;
		float Unknown32;
		COLORREF ColorStart;
		DWORD ColorEnd;
		GradientDirection Gradient;
		long TextureId;
	} drawTexturedRect =
	{
		RenderCommand::DrawTexturedRect,
		x0,
		y0,
		x1,
		y1,
		u0,
		v0,
		u1,
		v1,
		1.0f,
		colorStart,
		colorEnd,
		gradient,
		textureId
	};

	RenderBuffer.Put(&drawTexturedRect, sizeof(drawTexturedRect));
}

bool Overlay::DeleteTexture(long textureId) noexcept
{
	struct
	{
		RenderCommand CommandId;
		long TextureId;
	} command =
	{
		RenderCommand::DeleteTexture,
		textureId
	};

	if (!RenderBuffer.Put(&command, sizeof(command)))
		return false;

	AckNeeded = true;
	AckTextureId = textureId;
	AckVersion = 0;

	return true;
}

long Overlay::GetNewTextureId() noexcept
{
	return TextureIdCounter++;
}

void Overlay::InputThreadRoutine()
{
	char* buffer = new char[InputStream.GetCapacity()];

	HANDLE handles[] =
	{
		InputStream.GetMutexHandle(),
		InputStream.GetWrittenHandle()
	};

	while (!IsClosing)
	{
		if (WaitForMultipleObjects(ARRAYSIZE(handles), handles, TRUE, WaitTimeout) != WAIT_OBJECT_0)
			continue;

		DWORD size = InputStream.GetSize();

		if (size != 0)
		{
			InputStream.Get(buffer, size);

			for (DWORD i = 0; i < size;)
			{
				InputEvent eventId = *(InputEvent*)(buffer + i);
				i += 4;

				switch (eventId)
				{
				case InputEvent::SetGeneralInformation:
					i += 32;
					break;

				case InputEvent::TextureReceived:
					//printf("Loaded %d (%d).\n", *(long*)(buffer + i), *(long*)(buffer + i + 4));

					if (AckNeeded && *(long*)(buffer + i) == AckTextureId && *(long*)(buffer + i + 4) == AckVersion)
						AckNeeded = false;

					i += 8;
					break;

				case InputEvent::TextureDeleted:
				{
					long textureId = *(long*)(buffer + i);

					//printf("Deleted %d.\n", textureId);

					TextureVersions.erase(textureId);

					if (AckNeeded && AckVersion == 0 && AckTextureId == textureId)
						AckNeeded = false;

					i += 4;
					break;
				}

				case InputEvent::SetIngameFps:
					i += 4;
					break;

				case InputEvent::UpdateVrState:
					i += 2;
					break;

				case InputEvent::SetFpsBannerTextureId:
				case InputEvent::SetNotificationPosition:
					i += 4;
					break;

				case InputEvent::SetGameId:
				case InputEvent::SetScreenEdgePadding:
					i += 8;
					break;

				case InputEvent::Log:
					i += 256;
					break;

				case InputEvent::VrModeBegin:
					i += 260;
					break;

				case InputEvent::Unknown:
					i += 608;
					break;

				case InputEvent::SetImeLanguageList:
					i += 13639;
					break;
				}
			}
		}

		ReleaseMutex(handles[0]);
	}

	delete[] buffer;
}

TexturePage Overlay::CreateAndLoadNewTexturePage(LONG width, LONG height)
{
	long textureId = GetNewTextureId();

	if (!LoadTexture(textureId, 1, width * height * 4, width, height, 0, 0))
		throw std::runtime_error("Failed to load a new texture page.");

	return TexturePage(textureId, width, height);
}

bool Overlay::LoadTextureInternal(long textureId, long fullUpdate, DWORD size, DWORD width, DWORD height, long x, long y)
{
	struct LoadTextureCommand
	{
		RenderCommand CommandId;
		long TextureId;
		long Version; 
		long FullUpdate; 
		DWORD Size;
		DWORD Width;
		DWORD Height;
		long X;
		long Y;
	};

	// not enough room for the command
	if (sizeof(LoadTextureCommand) + size > RenderBuffer.GetUnusedCapacity())
		return false;

	long version;
	std::map<long, long>::iterator it = TextureVersions.find(textureId);

	if (it != TextureVersions.end())
	{
		version = ++it->second;
	}

	else
	{
		version = 1;
		TextureVersions.insert({ textureId, version });
	}

	LoadTextureCommand command =
	{
		RenderCommand::LoadTexture,
		textureId,
		version,
		fullUpdate,
		size,
		width,
		height,
		x,
		y
	};

	RenderBuffer.Put(&command, sizeof(command));

	AckNeeded = true;
	AckTextureId = textureId;
	AckVersion = version;

	return true;
}
