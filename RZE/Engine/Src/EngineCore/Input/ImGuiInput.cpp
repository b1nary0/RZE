#include <StdAfx.h>
#include <EngineCore/Input/ImGuiInput.h>

#include <EngineCore/Input/InputHandler.h>

namespace
{
	// Mirrors ImGui_ImplWin32_KeyEventToImGuiKey() from imgui_impl_win32.cpp, which isn't exported.
	ImGuiKey VirtualKeyToImGuiKey(int vk)
	{
		if (vk >= '0' && vk <= '9')
		{
			return static_cast<ImGuiKey>(ImGuiKey_0 + (vk - '0'));
		}
		if (vk >= 'A' && vk <= 'Z')
		{
			return static_cast<ImGuiKey>(ImGuiKey_A + (vk - 'A'));
		}
		if (vk >= KeyCode::F1 && vk <= KeyCode::F24)
		{
			return static_cast<ImGuiKey>(ImGuiKey_F1 + (vk - KeyCode::F1));
		}
		if (vk >= KeyCode::Numpad0 && vk <= KeyCode::Numpad9)
		{
			return static_cast<ImGuiKey>(ImGuiKey_Keypad0 + (vk - KeyCode::Numpad0));
		}

		switch (vk)
		{
		case KeyCode::Tab: return ImGuiKey_Tab;
		case KeyCode::LEFT: return ImGuiKey_LeftArrow;
		case KeyCode::Right: return ImGuiKey_RightArrow;
		case KeyCode::Up: return ImGuiKey_UpArrow;
		case KeyCode::Down: return ImGuiKey_DownArrow;
		case KeyCode::Prior: return ImGuiKey_PageUp;
		case KeyCode::Next: return ImGuiKey_PageDown;
		case KeyCode::Home: return ImGuiKey_Home;
		case KeyCode::End: return ImGuiKey_End;
		case KeyCode::Insert: return ImGuiKey_Insert;
		case KeyCode::Delete: return ImGuiKey_Delete;
		case KeyCode::Backspace: return ImGuiKey_Backspace;
		case KeyCode::Space: return ImGuiKey_Space;
		case KeyCode::Return: return ImGuiKey_Enter;
		case KeyCode::Escape: return ImGuiKey_Escape;
		case KeyCode::OEM_Comma: return ImGuiKey_Comma;
		case KeyCode::OEM_Period: return ImGuiKey_Period;
		case KeyCode::Capital: return ImGuiKey_CapsLock;
		case KeyCode::Scroll: return ImGuiKey_ScrollLock;
		case KeyCode::Numlock: return ImGuiKey_NumLock;
		case KeyCode::Snapshot: return ImGuiKey_PrintScreen;
		case KeyCode::Pause: return ImGuiKey_Pause;
		case KeyCode::Decimal: return ImGuiKey_KeypadDecimal;
		case KeyCode::Divide: return ImGuiKey_KeypadDivide;
		case KeyCode::Multiply: return ImGuiKey_KeypadMultiply;
		case KeyCode::Subtract: return ImGuiKey_KeypadSubtract;
		case KeyCode::Add: return ImGuiKey_KeypadAdd;
		// Left Shift/Ctrl/Alt are submitted separately in SubmitInput() since the generic KeyCode::Shift etc. alias them.
		case KeyCode::RShift: return ImGuiKey_RightShift;
		case KeyCode::RControl: return ImGuiKey_RightCtrl;
		case KeyCode::RMenu: return ImGuiKey_RightAlt;
		case KeyCode::LWin: return ImGuiKey_LeftSuper;
		case KeyCode::RWin: return ImGuiKey_RightSuper;
		case KeyCode::Apps: return ImGuiKey_Menu;
		default: return ImGuiKey_None;
		}
	}
}

namespace ImGuiInput
{
	void SubmitInput(const InputHandler& handler)
	{
		ImGuiIO& io = ImGui::GetIO();

		const auto& mouseState = handler.GetProxyMouseState();
		io.AddMousePosEvent(mouseState.CurPosition.X(), mouseState.CurPosition.Y());

		// EMouseButton's order is ImGui's: left, right, middle, then the side buttons (ImGui's 3 and 4)
		for (int mouseBtn = 0; mouseBtn < MAX_MOUSE_BTN_SUPPORTED; ++mouseBtn)
		{
			io.AddMouseButtonEvent(mouseBtn, mouseState.CurMouseBtnStates[mouseBtn]);
		}

		// CurWheelVal is the raw WM_MOUSEWHEEL delta for this frame (cleared next frame), ImGui wants notches.
		if (mouseState.CurWheelVal != 0)
		{
			io.AddMouseWheelEvent(0.0f, static_cast<float>(mouseState.CurWheelVal) / static_cast<float>(k_mouseWheelNotchDelta));
		}

		// ImGui drops events that don't change a key's state, so submitting every key each frame is fine.
		const auto& keyboardState = handler.GetProxyKeyboardState();
		for (int key = 0; key < MAX_KEYCODES_SUPPORTED; ++key)
		{
			const ImGuiKey imguiKey = VirtualKeyToImGuiKey(key);
			if (imguiKey != ImGuiKey_None)
			{
				io.AddKeyEvent(imguiKey, keyboardState.IsDownThisFrame(key));
			}
		}

		const bool ctrlDown = keyboardState.IsDownThisFrame(KeyCode::Control) || keyboardState.IsDownThisFrame(KeyCode::LControl);
		const bool shiftDown = keyboardState.IsDownThisFrame(KeyCode::Shift) || keyboardState.IsDownThisFrame(KeyCode::LShift);
		const bool altDown = keyboardState.IsDownThisFrame(KeyCode::Menu) || keyboardState.IsDownThisFrame(KeyCode::LMenu);
		io.AddKeyEvent(ImGuiKey_LeftCtrl, ctrlDown);
		io.AddKeyEvent(ImGuiKey_LeftShift, shiftDown);
		io.AddKeyEvent(ImGuiKey_LeftAlt, altDown);
		io.AddKeyEvent(ImGuiMod_Ctrl, ctrlDown || keyboardState.IsDownThisFrame(KeyCode::RControl));
		io.AddKeyEvent(ImGuiMod_Shift, shiftDown || keyboardState.IsDownThisFrame(KeyCode::RShift));
		io.AddKeyEvent(ImGuiMod_Alt, altDown || keyboardState.IsDownThisFrame(KeyCode::RMenu));
	}
}
