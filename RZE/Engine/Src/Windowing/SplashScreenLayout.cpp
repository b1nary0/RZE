#include <StdAfx.h>

#include <Windowing/SplashScreenLayout.h>

#include <Windowing/WindowIcon.h>

#include <algorithm>
#include <cmath>

namespace SplashScreenLayout
{
	namespace
	{
		// Adds weight of image's texel at (x, y) to premultiplied: red, green and blue premultiplied by alpha, then alpha,
		// all out of 255
		void AddTexel(const IconImage& image, int x, int y, double weight, double(&premultiplied)[4])
		{
			const size_t index = (static_cast<size_t>(y) * image.Width + x) * k_bytesPerTexel;
			const double alpha = image.Texels[index + 3];
			for (int channel = 0; channel < 3; ++channel)
			{
				premultiplied[channel] += weight * image.Texels[index + channel] * alpha / 255.0;
			}
			premultiplied[3] += weight * alpha;
		}
	}

	void DrawImage(const IconImage& image, int left, int top, int width, int height, std::vector<U8>& canvas, int canvasWidth, int canvasHeight)
	{
		const double scaleX = static_cast<double>(image.Width) / width;
		const double scaleY = static_cast<double>(image.Height) / height;

		for (int y = std::max(0, -top); y < height && top + y < canvasHeight; ++y)
		{
			const double sourceY = std::clamp((y + 0.5) * scaleY - 0.5, 0.0, image.Height - 1.0);
			const int y0 = static_cast<int>(sourceY);
			const int y1 = std::min(y0 + 1, image.Height - 1);
			const double fractionY = sourceY - y0;

			for (int x = std::max(0, -left); x < width && left + x < canvasWidth; ++x)
			{
				const double sourceX = std::clamp((x + 0.5) * scaleX - 0.5, 0.0, image.Width - 1.0);
				const int x0 = static_cast<int>(sourceX);
				const int x1 = std::min(x0 + 1, image.Width - 1);
				const double fractionX = sourceX - x0;

				// Premultiplied, so transparent texels don't darken the edges
				double premultiplied[4] = {};
				AddTexel(image, x0, y0, (1.0 - fractionX) * (1.0 - fractionY), premultiplied);
				AddTexel(image, x1, y0, fractionX * (1.0 - fractionY), premultiplied);
				AddTexel(image, x0, y1, (1.0 - fractionX) * fractionY, premultiplied);
				AddTexel(image, x1, y1, fractionX * fractionY, premultiplied);

				U8* const destination = &canvas[(static_cast<size_t>(top + y) * canvasWidth + left + x) * k_bytesPerTexel];
				const double remaining = 1.0 - premultiplied[3] / 255.0;
				destination[0] = static_cast<U8>(std::lround(premultiplied[2] + destination[0] * remaining));
				destination[1] = static_cast<U8>(std::lround(premultiplied[1] + destination[1] * remaining));
				destination[2] = static_cast<U8>(std::lround(premultiplied[0] + destination[2] * remaining));
			}
		}
	}
}
