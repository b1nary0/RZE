#pragma once

#include <Windows.h>

#include <Utils/PrimitiveDefs.h>

#include <memory>
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

// A window's large (taskbar, Alt+Tab) and small (system menu) icons, made from an image file. They're destroyed with
// it, so it has to outlive the window's use of them.
class WindowIcon
{
public:
	~WindowIcon();

	WindowIcon(const WindowIcon&) = delete;
	WindowIcon& operator=(const WindowIcon&) = delete;

	// The image's icons at the system's icon sizes for dpi; null if the image can't be read
	static std::unique_ptr<WindowIcon> FromImage(const Filepath& image, UINT dpi);

	// Empty if the file can't be read as an image
	static IconImage ReadImage(const Filepath& image);
	// image made square, centred on transparent padding, then scaled to size x size: each texel averages the part of
	// the image it covers, weighted by alpha so transparent texels don't darken the edges
	static IconImage MakeSquare(const IconImage& image, int size);

	HICON GetLarge() const { return m_large; }
	HICON GetSmall() const { return m_small; }

private:
	WindowIcon(HICON largeIcon, HICON smallIcon);

	HICON m_large = nullptr;
	HICON m_small = nullptr;
};
