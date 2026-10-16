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

TEST_CASE("The input event count goes up for real input only")
{
	InputHandler handler;
	handler.Initialize();
	U64 count = handler.GetInputEventCount();

	handler.OnMouseMove(10, 20);
	CHECK(handler.GetInputEventCount() == ++count);

	// The same position again, a wheel message with no turn, and clearing the wheel aren't input
	handler.OnMouseMove(10, 20);
	handler.OnMouseWheel(0);
	handler.ClearMouseWheel();
	CHECK(handler.GetInputEventCount() == count);

	handler.OnMouseWheel(120);
	handler.OnMouseDown(EMouseButton::MouseButton_Left, 10, 20);
	handler.OnMouseUp(EMouseButton::MouseButton_Left, 10, 20);
	handler.OnKeyDown('W', false);
	handler.OnKeyUp('W');
	CHECK(handler.GetInputEventCount() == count + 5);
}

TEST_CASE("Releasing all input lets go of every key and button still down, as releases")
{
	InputHandler handler;
	handler.Initialize();

	handler.OnMouseMove(10, 20);
	handler.OnKeyDown('W', false);
	handler.OnKeyDown(0x10, false); // Shift
	handler.OnMouseDown(EMouseButton::MouseButton_Right, 10, 20);
	CHECK_FALSE(handler.GetProxyKeyboardState().IsIdle());
	CHECK_FALSE(handler.GetProxyMouseState().IsIdle());

	const U64 count = handler.GetInputEventCount();
	handler.ReleaseAll();
	CHECK(handler.GetProxyKeyboardState().IsIdle());
	CHECK(handler.GetProxyMouseState().IsIdle());
	// One release each
	CHECK(handler.GetInputEventCount() == count + 3);

	// Nothing left to let go of
	handler.ReleaseAll();
	CHECK(handler.GetInputEventCount() == count + 3);
}

TEST_CASE("A frame with input, or with a mouse button held while focused, is interacting")
{
	InputHandler handler;
	handler.Initialize();

	handler.UpdateFrameState(true, false);
	CHECK_FALSE(handler.IsInteracting());

	handler.OnMouseMove(10, 20);
	handler.UpdateFrameState(true, false);
	CHECK(handler.IsInteracting());

	// The input was last frame's
	handler.UpdateFrameState(true, false);
	CHECK_FALSE(handler.IsInteracting());

	handler.UpdateFrameState(true, true);
	CHECK(handler.IsInteracting());

	// A button held over another window isn't the user working in this one
	handler.UpdateFrameState(false, true);
	handler.UpdateFrameState(false, true);
	CHECK_FALSE(handler.IsInteracting());
}

TEST_CASE("Losing focus lets go of everything still down")
{
	InputHandler handler;
	handler.Initialize();
	handler.UpdateFrameState(true, false);

	handler.OnKeyDown('W', false);
	handler.OnMouseDown(EMouseButton::MouseButton_Right, 10, 20);
	handler.UpdateFrameState(true, true);
	CHECK_FALSE(handler.GetProxyKeyboardState().IsIdle());

	handler.UpdateFrameState(false, false);
	CHECK(handler.GetProxyKeyboardState().IsIdle());
	CHECK(handler.GetProxyMouseState().IsIdle());
}
