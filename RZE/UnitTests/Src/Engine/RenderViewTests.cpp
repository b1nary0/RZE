#include <doctest/doctest.h>

#include <Graphics/RenderView.h>

TEST_CASE("A view's frame requests are cleared together")
{
	RenderView view(ERenderViewKind::Main);
	view.Frame.Pick = RenderPickRequest();
	view.Frame.Outlines.push_back(RenderOutline());

	view.ClearFrameRequests();

	CHECK_FALSE(view.Frame.Pick.has_value());
	CHECK(view.Frame.Outlines.empty());
}
