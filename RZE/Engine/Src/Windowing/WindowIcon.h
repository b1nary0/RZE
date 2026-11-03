#pragma once

#include <Utils/PrimitiveDefs.h>

#include <vector>

class Filepath;

// An image's texels as 8-bit RGBA rows, top row first
struct IconImage
{
	std::vector<U8> Texels;
	int Width = 0;
	int Height = 0;

	bool IsEmpty() const { return Texels.empty(); }
};

// Making a window's icon from an image file, the same on every platform. The OS's icon objects are made from these by
// each platform's window (e.g. Windowing/Windows/WindowIconWindows.h).
class WindowIcon
{
public:
	WindowIcon() = delete;

	// Empty if the file can't be read as an image
	static IconImage ReadImage(const Filepath& image);
	// image made square, centred on transparent padding, then scaled to size x size: each texel averages the part of
	// the image it covers, weighted by alpha so transparent texels don't darken the edges
	static IconImage MakeSquare(const IconImage& image, int size);
};
