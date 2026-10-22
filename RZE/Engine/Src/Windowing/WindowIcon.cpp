#include <StdAfx.h>

#include <Windowing/WindowIcon.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Platform/Filepath.h>

#include <STB/stb_image.h>

#include <algorithm>
#include <cmath>

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

	// How much of [min, max) falls on texel index (which covers [index, index + 1))
	double Overlap(double min, double max, int index)
	{
		return std::max(0.0, std::min(max, index + 1.0) - std::max(min, static_cast<double>(index)));
	}
}

WindowIcon::WindowIcon(HICON largeIcon, HICON smallIcon)
	: m_large(largeIcon)
	, m_small(smallIcon)
{
}

WindowIcon::~WindowIcon()
{
	DestroyIcon(m_large);
	DestroyIcon(m_small);
}

std::unique_ptr<WindowIcon> WindowIcon::FromImage(const Filepath& image, UINT dpi)
{
	const IconImage source = ReadImage(image);
	if (source.IsEmpty())
	{
		RZE_LOG_WARNING_ARGS("Couldn't read the window icon [%s]; keeping the executable's icon.", image.GetRelativePath().c_str());
		return nullptr;
	}

	const HICON largeIcon = CreateIconFromTexels(MakeSquare(source, GetSystemMetricsForDpi(SM_CXICON, dpi)));
	const HICON smallIcon = CreateIconFromTexels(MakeSquare(source, GetSystemMetricsForDpi(SM_CXSMICON, dpi)));
	if (largeIcon == nullptr || smallIcon == nullptr)
	{
		RZE_LOG_WARNING_ARGS("Couldn't make the window icon from [%s]; keeping the executable's icon.", image.GetRelativePath().c_str());
		DestroyIcon(largeIcon);
		DestroyIcon(smallIcon);
		return nullptr;
	}

	return std::unique_ptr<WindowIcon>(new WindowIcon(largeIcon, smallIcon));
}

IconImage WindowIcon::ReadImage(const Filepath& image)
{
	IconImage loaded;
	int channels = 0;
	stbi_uc* const texels = stbi_load(image.GetAbsolutePath().c_str(), &loaded.Width, &loaded.Height, &channels, STBI_rgb_alpha);
	if (texels == nullptr)
	{
		return IconImage();
	}

	loaded.Texels.assign(texels, texels + static_cast<size_t>(loaded.Width) * loaded.Height * k_bytesPerTexel);
	stbi_image_free(texels);
	return loaded;
}

IconImage WindowIcon::MakeSquare(const IconImage& image, int size)
{
	IconImage square;
	square.Width = size;
	square.Height = size;
	square.Texels.assign(static_cast<size_t>(size) * size * k_bytesPerTexel, 0);

	// In the image's texels, where the square's sides are and how much of it each output texel covers
	const int side = std::max(image.Width, image.Height);
	const double left = (side - image.Width) * 0.5;
	const double top = (side - image.Height) * 0.5;
	const double step = static_cast<double>(side) / size;

	for (int y = 0; y < size; ++y)
	{
		const double minY = y * step - top;
		const double maxY = minY + step;
		const int firstRow = std::max(0, static_cast<int>(std::floor(minY)));
		const int lastRow = std::min(image.Height - 1, static_cast<int>(std::ceil(maxY)) - 1);

		for (int x = 0; x < size; ++x)
		{
			const double minX = x * step - left;
			const double maxX = minX + step;
			const int firstColumn = std::max(0, static_cast<int>(std::floor(minX)));
			const int lastColumn = std::min(image.Width - 1, static_cast<int>(std::ceil(maxX)) - 1);

			// Colours weighted by alpha as well as coverage; the padding counts towards the area with no alpha
			double colour[3] = { 0.0, 0.0, 0.0 };
			double alpha = 0.0;
			for (int row = firstRow; row <= lastRow; ++row)
			{
				const double rowWeight = Overlap(minY, maxY, row);
				for (int column = firstColumn; column <= lastColumn; ++column)
				{
					const U8* const texel = &image.Texels[(static_cast<size_t>(row) * image.Width + column) * k_bytesPerTexel];
					const double weight = rowWeight * Overlap(minX, maxX, column) * texel[3];
					colour[0] += texel[0] * weight;
					colour[1] += texel[1] * weight;
					colour[2] += texel[2] * weight;
					alpha += weight;
				}
			}

			if (alpha <= 0.0)
			{
				continue;
			}

			U8* const output = &square.Texels[(static_cast<size_t>(y) * size + x) * k_bytesPerTexel];
			for (int channel = 0; channel < 3; ++channel)
			{
				output[channel] = static_cast<U8>(std::lround(std::min(255.0, colour[channel] / alpha)));
			}
			output[3] = static_cast<U8>(std::lround(std::min(255.0, alpha / (step * step))));
		}
	}

	return square;
}
