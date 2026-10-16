#include <doctest/doctest.h>

#include <EngineCore/Input/InputHandler.h>

TEST_CASE("Wheel messages in the same frame add up, and clearing the wheel starts the next frame from zero")
{
	InputHandler handler;
	handler.Initialize();

	handler.OnMouseWheel(120);
	handler.OnMouseWheel(120);
	CHECK(handler.GetProxyMouseState().CurWheelVal == 240);

	handler.ClearMouseWheel();
	CHECK(handler.GetProxyMouseState().CurWheelVal == 0);

	handler.OnMouseWheel(-120);
	CHECK(handler.GetProxyMouseState().CurWheelVal == -120);
}
