#include <StdAfx.h>

#include <Windowing/Windows/WindowIconWindows.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Platform/Filepath.h>

#include <vector>

namespace
{
	constexpr int k_bytesPerTexel = 4;

	// An icon from size x size RGBA texels: a 32-bit colour bitmap whose alpha does the masking, and an empty mask
	HICON CreateIconFromTexels(const IconImage& image)
	{
		BITMAPV5HEADER header = {};
		header.bV5Size = sizeof(header);
		header.bV5Width = image.Width;
		// Negative for top row first, as the texels are
		header.bV5Height = -image.Height;
		header.bV5Planes = 1;
		header.bV5BitCount = 32;
		header.bV5Compression = BI_BITFIELDS;
		header.bV5RedMask = 0x00FF0000;
		header.bV5GreenMask = 0x0000FF00;
		header.bV5BlueMask = 0x000000FF;
		header.bV5AlphaMask = 0xFF000000;

		void* bits = nullptr;
		const HDC screen = GetDC(nullptr);
		const HBITMAP colour = CreateDIBSection(screen, reinterpret_cast<const BITMAPINFO*>(&header), DIB_RGB_COLORS, &bits, nullptr, 0);
		ReleaseDC(nullptr, screen);
		if (colour == nullptr)
		{
			return nullptr;
		}

		// BGRA, not premultiplied
		U8* const destination = static_cast<U8*>(bits);
		for (size_t texel = 0; texel < image.Texels.size(); texel += k_bytesPerTexel)
		{
			destination[texel + 0] = image.Texels[texel + 2];
			destination[texel + 1] = image.Texels[texel + 1];
			destination[texel + 2] = image.Texels[texel + 0];
			destination[texel + 3] = image.Texels[texel + 3];
		}

		// 1 bit per texel, rows padded to 16 bits
		const std::vector<U8> maskBits(static_cast<size_t>((image.Width + 15) / 16) * 2 * image.Height, 0);
		const HBITMAP mask = CreateBitmap(image.Width, image.Height, 1, 1, maskBits.data());

		ICONINFO info = {};
		info.fIcon = TRUE;
		info.hbmMask = mask;
		info.hbmColor = colour;
		const HICON icon = CreateIconIndirect(&info);

		// The icon has its own copies
		DeleteObject(mask);
		DeleteObject(colour);
		return icon;
	}
}

WindowIconWindows::WindowIconWindows(HICON largeIcon, HICON smallIcon)
	: m_large(largeIcon)
	, m_small(smallIcon)
{
}

WindowIconWindows::~WindowIconWindows()
{
	DestroyIcon(m_large);
	DestroyIcon(m_small);
}

std::unique_ptr<WindowIconWindows> WindowIconWindows::FromImage(const Filepath& image, UINT dpi)
{
	const IconImage source = WindowIcon::ReadImage(image);
	if (source.IsEmpty())
	{
		RZE_LOG_WARNING_ARGS("Couldn't read the window icon [%s]; keeping the executable's icon.", image.GetRelativePath().c_str());
		return nullptr;
	}

	const HICON largeIcon = CreateIconFromTexels(WindowIcon::MakeSquare(source, GetSystemMetricsForDpi(SM_CXICON, dpi)));
	const HICON smallIcon = CreateIconFromTexels(WindowIcon::MakeSquare(source, GetSystemMetricsForDpi(SM_CXSMICON, dpi)));
	if (largeIcon == nullptr || smallIcon == nullptr)
	{
		RZE_LOG_WARNING_ARGS("Couldn't make the window icon from [%s]; keeping the executable's icon.", image.GetRelativePath().c_str());
		DestroyIcon(largeIcon);
		DestroyIcon(smallIcon);
		return nullptr;
	}

	return std::unique_ptr<WindowIconWindows>(new WindowIconWindows(largeIcon, smallIcon));
}
