#include <doctest/doctest.h>

#include <Windowing/TitleBarLayout.h>

namespace
{
	constexpr int k_clientWidth = 1000;
	constexpr int k_resizeBorder = 8;

	Display::ScreenRect MakeRect(int left, int top, int right, int bottom)
	{
		Display::ScreenRect rect;
		rect.Left = left;
		rect.Top = top;
		rect.Right = right;
		rect.Bottom = bottom;
		return rect;
	}

	// A 31-pixel bar with menus at 50-200 and the maximize button at 908-954
	TitleBarLayout MakeLayout()
	{
		TitleBarLayout layout;
		layout.Height = 31;
		layout.Controls.push_back(MakeRect(50, 0, 200, 31));
		layout.MaximizeButton = MakeRect(908, 0, 954, 31);
		return layout;
	}

	ETitleBarHit HitTest(const TitleBarLayout& layout, int x, int y, bool isMaximized = false)
	{
		return layout.HitTest(x, y, k_clientWidth, isMaximized, k_resizeBorder);
	}
}

TEST_CASE("The title bar moves the window, except where its controls take the click")
{
	const TitleBarLayout layout = MakeLayout();

	CHECK(HitTest(layout, 20, 15) == ETitleBarHit::Caption);
	CHECK(HitTest(layout, 500, 15) == ETitleBarHit::Caption);
	CHECK(HitTest(layout, 50, 15) == ETitleBarHit::Client);
	CHECK(HitTest(layout, 199, 30) == ETitleBarHit::Client);
	// Right and Bottom are just outside a control
	CHECK(HitTest(layout, 200, 15) == ETitleBarHit::Caption);
}

TEST_CASE("The maximize button is reported as one, so the OS can offer Snap Layouts over it")
{
	const TitleBarLayout layout = MakeLayout();

	CHECK(HitTest(layout, 908, 15) == ETitleBarHit::MaximizeButton);
	CHECK(HitTest(layout, 953, 30) == ETitleBarHit::MaximizeButton);
	CHECK(HitTest(layout, 954, 15) == ETitleBarHit::Caption);
}

TEST_CASE("The top edge of the title bar resizes the window, at the corners diagonally")
{
	const TitleBarLayout layout = MakeLayout();

	CHECK(HitTest(layout, 500, 0) == ETitleBarHit::ResizeTop);
	CHECK(HitTest(layout, 500, k_resizeBorder - 1) == ETitleBarHit::ResizeTop);
	CHECK(HitTest(layout, 500, k_resizeBorder) == ETitleBarHit::Caption);
	CHECK(HitTest(layout, 0, 0) == ETitleBarHit::ResizeTopLeft);
	CHECK(HitTest(layout, k_clientWidth - 1, 0) == ETitleBarHit::ResizeTopRight);
	// Even over a control or the maximize button
	CHECK(HitTest(layout, 100, 2) == ETitleBarHit::ResizeTop);
	CHECK(HitTest(layout, 920, 2) == ETitleBarHit::ResizeTop);
}

TEST_CASE("A maximized window doesn't resize from the title bar's top edge")
{
	const TitleBarLayout layout = MakeLayout();

	CHECK(HitTest(layout, 500, 0, true) == ETitleBarHit::Caption);
	CHECK(HitTest(layout, 0, 0, true) == ETitleBarHit::Caption);
	CHECK(HitTest(layout, 920, 0, true) == ETitleBarHit::MaximizeButton);
}

TEST_CASE("Below the title bar, and with no bar at all, everything is client area")
{
	CHECK(HitTest(MakeLayout(), 500, 31) == ETitleBarHit::Client);
	CHECK(HitTest(MakeLayout(), 920, 400) == ETitleBarHit::Client);
	CHECK(HitTest(TitleBarLayout(), 500, 0) == ETitleBarHit::Client);
}
