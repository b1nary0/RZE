#pragma once

#include <Utils/PrimitiveDefs.h>

#include <vector>

struct IconImage;

// How the splash screen looks, the same on every platform (Windows/SplashScreenWindows.cpp, Linux/SplashScreenLinux.cpp).
// Only they use it.
namespace SplashScreenLayout
{
	struct Colour
	{
		U8 R;
		U8 G;
		U8 B;
	};

	// At 96 DPI. The image's texels are pixels at 96 DPI too, so everything scales with the DPI together.
	constexpr int k_width = 400;
	constexpr int k_height = 225;
	constexpr int k_statusMargin = 14;
	constexpr int k_statusPointSize = 9;
	// Under the status, which leaves room for it whether it's shown or not
	constexpr int k_progressBarHeight = 3;
	constexpr int k_progressBarGap = 6;

	// The Editor's window background (EditorApp::StyleSetup's WindowBg)
	constexpr Colour k_backgroundColour = { 33, 33, 33 };
	constexpr Colour k_statusColour = { 150, 150, 150 };
	// The Editor's progress bars (EditorApp::StyleSetup's PlotHistogram) on a track a little lighter than the background
	constexpr Colour k_progressColour = { 230, 179, 0 };
	constexpr Colour k_progressTrackColour = { 55, 55, 55 };

	// SetStatus without a progress bar
	constexpr float k_noProgress = -1.0f;

	constexpr int k_bytesPerTexel = 4;

	// image scaled to width x height (bilinear) and blended over canvas at (left, top). canvas is opaque BGRA texels,
	// top row first.
	void DrawImage(const IconImage& image, int left, int top, int width, int height, std::vector<U8>& canvas, int canvasWidth, int canvasHeight);
}
