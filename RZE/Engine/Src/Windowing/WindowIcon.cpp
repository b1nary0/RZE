#include <StdAfx.h>

#include <Windowing/WindowIcon.h>

#include <Utils/Platform/Filepath.h>

#include <STB/stb_image.h>

#include <algorithm>
#include <cmath>

namespace
{
	constexpr int k_bytesPerTexel = 4;

	// How much of [min, max) falls on texel index (which covers [index, index + 1))
	double Overlap(double min, double max, int index)
	{
		return std::max(0.0, std::min(max, index + 1.0) - std::max(min, static_cast<double>(index)));
	}
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
