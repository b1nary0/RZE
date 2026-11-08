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
	handler.OnKeyDown('W', false);
	// A frame on, so the releases aren't deferred
	handler.BeginFrame();
	handler.OnMouseUp(EMouseButton::MouseButton_Left, 10, 20);
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

namespace
{
	// One frame as the engine runs it: a new frame's input, the window's events, then the app's snapshot of the devices
	// the gate lets through (both by default)
	template <typename TEvents>
	void RunFrame(InputHandler& handler, TEvents&& events, const InputGate& gate = InputGate{})
	{
		handler.BeginFrame();
		events();
		handler.RaiseEvents(gate);
	}

	void RunFrame(InputHandler& handler)
	{
		RunFrame(handler, [] {});
	}
}

TEST_CASE("A held key reads Pressed for one frame, then Hold, whether or not the OS repeats it")
{
	InputHandler handler;
	handler.Initialize();

	RunFrame(handler, [&handler] { handler.OnKeyDown('W', false); });
	CHECK(handler.GetKeyboardState().GetButtonState('W') == EButtonState::ButtonState_Pressed);
	CHECK_FALSE(handler.GetKeyboardState().WasDownLastFrame('W'));

	RunFrame(handler);
	CHECK(handler.GetKeyboardState().GetButtonState('W') == EButtonState::ButtonState_Hold);
	CHECK(handler.GetKeyboardState().WasDownLastFrame('W'));

	RunFrame(handler, [&handler] { handler.OnKeyDown('W', true); });
	CHECK(handler.GetKeyboardState().GetButtonState('W') == EButtonState::ButtonState_Hold);

	RunFrame(handler, [&handler] { handler.OnKeyUp('W'); });
	CHECK(handler.GetKeyboardState().GetButtonState('W') == EButtonState::ButtonState_Released);
	CHECK(handler.GetKeyboardState().WasDownLastFrame('W'));
}

TEST_CASE("A held mouse button reads Pressed for one frame, then Hold")
{
	InputHandler handler;
	handler.Initialize();

	RunFrame(handler, [&handler] { handler.OnMouseDown(EMouseButton::MouseButton_Right, 10, 20); });
	CHECK(handler.GetMouseState().GetButtonState(EMouseButton::MouseButton_Right) == EButtonState::ButtonState_Pressed);
	CHECK(handler.GetMouseState().IsDownThisFrame(EMouseButton::MouseButton_Right));

	RunFrame(handler);
	CHECK(handler.GetMouseState().GetButtonState(EMouseButton::MouseButton_Right) == EButtonState::ButtonState_Hold);

	RunFrame(handler, [&handler] { handler.OnMouseUp(EMouseButton::MouseButton_Right, 10, 20); });
	CHECK(handler.GetMouseState().GetButtonState(EMouseButton::MouseButton_Right) == EButtonState::ButtonState_Released);
}

TEST_CASE("A key or button released in the frame it was pressed in is seen down for that frame")
{
	InputHandler handler;
	handler.Initialize();

	RunFrame(handler, [&handler]
		{
			handler.OnKeyDown('W', false);
			handler.OnKeyUp('W');
			handler.OnMouseDown(EMouseButton::MouseButton_Left, 10, 20);
			handler.OnMouseUp(EMouseButton::MouseButton_Left, 30, 40);
		});
	CHECK(handler.GetKeyboardState().GetButtonState('W') == EButtonState::ButtonState_Pressed);
	CHECK(handler.GetMouseState().GetButtonState(EMouseButton::MouseButton_Left) == EButtonState::ButtonState_Pressed);
	// ImGui reads the proxies, so they keep it down too
	CHECK(handler.GetProxyKeyboardState().IsDownThisFrame('W'));
	CHECK(handler.GetProxyMouseState().IsDownThisFrame(EMouseButton::MouseButton_Left));

	// Let go at the start of the next frame, and that counts as input then
	const U64 count = handler.GetInputEventCount();
	RunFrame(handler);
	CHECK(handler.GetKeyboardState().GetButtonState('W') == EButtonState::ButtonState_Released);
	CHECK(handler.GetMouseState().GetButtonState(EMouseButton::MouseButton_Left) == EButtonState::ButtonState_Released);
	CHECK(handler.GetInputEventCount() == count + 2);

	// Released in a later frame than the press: at once
	RunFrame(handler, [&handler] { handler.OnKeyDown('W', false); });
	handler.BeginFrame();
	handler.OnKeyUp('W');
	CHECK_FALSE(handler.GetProxyKeyboardState().IsDownThisFrame('W'));
}

TEST_CASE("A key pressed again before its deferred release stays down")
{
	InputHandler handler;
	handler.Initialize();

	RunFrame(handler, [&handler]
		{
			handler.OnKeyDown('W', false);
			handler.OnKeyUp('W');
			handler.OnKeyDown('W', false);
		});
	RunFrame(handler);
	RunFrame(handler);
	CHECK(handler.GetKeyboardState().GetButtonState('W') == EButtonState::ButtonState_Hold);
}

TEST_CASE("Losing focus lets go at once, even of what was pressed this frame")
{
	InputHandler handler;
	handler.Initialize();

	handler.BeginFrame();
	handler.OnKeyDown('W', false);
	handler.OnKeyUp('W');
	handler.ReleaseAll();
	CHECK(handler.GetProxyKeyboardState().IsIdle());

	// Nothing left over for the next frame to let go of
	const U64 count = handler.GetInputEventCount();
	handler.BeginFrame();
	CHECK(handler.GetInputEventCount() == count);
}

TEST_CASE("A Pressed binding runs once a press, not for the OS's repeats, even when they arrive in the same frame")
{
	InputHandler handler;
	handler.Initialize();

	int pressCount = 0;
	handler.BindAction('E', EButtonState::ButtonState_Pressed, Functor<void, const InputKey&>([&pressCount](const InputKey&) { ++pressCount; }));

	// A stalled frame: the press and its first repeats come together
	RunFrame(handler, [&handler]
		{
			handler.OnKeyDown('E', false);
			handler.OnKeyDown('E', true);
			handler.OnKeyDown('E', true);
		});
	CHECK(pressCount == 1);

	RunFrame(handler, [&handler] { handler.OnKeyDown('E', true); });
	CHECK(pressCount == 1);

	RunFrame(handler, [&handler] { handler.OnKeyUp('E'); });
	RunFrame(handler, [&handler] { handler.OnKeyDown('E', false); });
	CHECK(pressCount == 2);
}

TEST_CASE("Each device's input reaches the app only in frames the app takes that device")
{
	InputHandler handler;
	handler.Initialize();

	int keyCount = 0;
	int clickCount = 0;
	handler.BindAction('E', EButtonState::ButtonState_Pressed, Functor<void, const InputKey&>([&keyCount](const InputKey&) { ++keyCount; }));
	handler.BindMouseAction(EMouseButton::MouseButton_Left, EButtonState::ButtonState_Pressed, InputHandler::MouseActionFunc([&clickCount](Int32, Int32) { ++clickCount; }));

	const InputGate keyboardOnly{ true, false };
	const InputGate mouseOnly{ false, true };

	// The keyboard goes through, the mouse is held back: e.g. the Game with the cursor over an ImGui window
	RunFrame(handler, [&handler]
		{
			handler.OnKeyDown('E', false);
			handler.OnMouseDown(EMouseButton::MouseButton_Left, 10, 20);
		}, keyboardOnly);
	CHECK(keyCount == 1);
	CHECK(clickCount == 0);
	CHECK(handler.GetKeyboardState().GetButtonState('E') == EButtonState::ButtonState_Pressed);
	CHECK(handler.GetMouseState().IsIdle());

	// Both still held. The mouse is taken now: its button reads as just pressed, but the press was dropped with its
	// frame, so its binding doesn't fire late. The keyboard is held back and reads idle.
	RunFrame(handler, [] {}, mouseOnly);
	CHECK(clickCount == 0);
	CHECK(handler.GetMouseState().GetButtonState(EMouseButton::MouseButton_Left) == EButtonState::ButtonState_Pressed);
	CHECK(handler.GetKeyboardState().IsIdle());
	CHECK(keyCount == 1);

	// Both taken: the key, held back last frame, reads as just pressed again; the button has been seen for a frame
	RunFrame(handler);
	CHECK(handler.GetKeyboardState().GetButtonState('E') == EButtonState::ButtonState_Pressed);
	CHECK(handler.GetMouseState().GetButtonState(EMouseButton::MouseButton_Left) == EButtonState::ButtonState_Hold);
	CHECK(keyCount == 1);
	CHECK(clickCount == 0);
}

TEST_CASE("A wheel turn isn't read again once the app stops taking the mouse")
{
	InputHandler handler;
	handler.Initialize();

	RunFrame(handler, [&handler] { handler.OnMouseWheel(120); });
	CHECK(handler.GetMouseState().CurWheelVal == 120);

	// The window clears the wheel at the start of each frame
	RunFrame(handler, [&handler] { handler.ClearMouseWheel(); }, InputGate{ true, false });
	CHECK(handler.GetMouseState().CurWheelVal == 0);
}

TEST_CASE("The pointer's delta is this frame's movement, however many moves arrive, and the first position isn't a move")
{
	InputHandler handler;
	handler.Initialize();

	// The first position the window reports, far from the placeholder (0, 0)
	RunFrame(handler, [&handler] { handler.OnMouseMove(800, 450); });
	CHECK(handler.GetMouseState().GetPositionDelta().X() == 0.0f);
	CHECK(handler.GetMouseState().GetPositionDelta().Y() == 0.0f);

	RunFrame(handler, [&handler]
		{
			handler.OnMouseMove(810, 455);
			handler.OnMouseMove(820, 460);
		});
	CHECK(handler.GetMouseState().GetPositionDelta().X() == 20.0f);
	CHECK(handler.GetMouseState().GetPositionDelta().Y() == 10.0f);
	CHECK(handler.GetMouseState().PrevPosition.X() == 800.0f);

	// No moves: no delta
	RunFrame(handler);
	CHECK(handler.GetMouseState().GetPositionDelta().X() == 0.0f);
	CHECK(handler.GetMouseState().GetPositionDelta().Y() == 0.0f);
}

TEST_CASE("While the app doesn't take the mouse the pointer doesn't move as far as it can tell, and taking it back isn't a jump")
{
	InputHandler handler;
	handler.Initialize();

	const InputGate keyboardOnly{ true, false };

	// Held back from the first position on
	RunFrame(handler, [&handler] { handler.OnMouseMove(100, 100); }, keyboardOnly);
	RunFrame(handler, [&handler] { handler.OnMouseMove(300, 200); }, keyboardOnly);
	CHECK(handler.GetMouseState().GetPositionDelta().X() == 0.0f);
	CHECK(handler.GetMouseState().GetPositionDelta().Y() == 0.0f);
	// The position is kept current all the same
	CHECK(handler.GetMouseState().CurPosition.X() == 300.0f);

	// Taken again: only this frame's movement
	RunFrame(handler, [&handler] { handler.OnMouseMove(310, 200); });
	CHECK(handler.GetMouseState().GetPositionDelta().X() == 10.0f);
	CHECK(handler.GetMouseState().GetPositionDelta().Y() == 0.0f);
}
