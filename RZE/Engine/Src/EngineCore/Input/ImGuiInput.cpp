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
		if (vk >= VK_F1 && vk <= VK_F24)
		{
			return static_cast<ImGuiKey>(ImGuiKey_F1 + (vk - VK_F1));
		}
		if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9)
		{
			return static_cast<ImGuiKey>(ImGuiKey_Keypad0 + (vk - VK_NUMPAD0));
		}

		switch (vk)
		{
		case VK_TAB: return ImGuiKey_Tab;
		case VK_LEFT: return ImGuiKey_LeftArrow;
		case VK_RIGHT: return ImGuiKey_RightArrow;
		case VK_UP: return ImGuiKey_UpArrow;
		case VK_DOWN: return ImGuiKey_DownArrow;
		case VK_PRIOR: return ImGuiKey_PageUp;
		case VK_NEXT: return ImGuiKey_PageDown;
		case VK_HOME: return ImGuiKey_Home;
		case VK_END: return ImGuiKey_End;
		case VK_INSERT: return ImGuiKey_Insert;
		case VK_DELETE: return ImGuiKey_Delete;
		case VK_BACK: return ImGuiKey_Backspace;
		case VK_SPACE: return ImGuiKey_Space;
		case VK_RETURN: return ImGuiKey_Enter;
		case VK_ESCAPE: return ImGuiKey_Escape;
		case VK_OEM_COMMA: return ImGuiKey_Comma;
		case VK_OEM_PERIOD: return ImGuiKey_Period;
		case VK_CAPITAL: return ImGuiKey_CapsLock;
		case VK_SCROLL: return ImGuiKey_ScrollLock;
		case VK_NUMLOCK: return ImGuiKey_NumLock;
		case VK_SNAPSHOT: return ImGuiKey_PrintScreen;
		case VK_PAUSE: return ImGuiKey_Pause;
		case VK_DECIMAL: return ImGuiKey_KeypadDecimal;
		case VK_DIVIDE: return ImGuiKey_KeypadDivide;
		case VK_MULTIPLY: return ImGuiKey_KeypadMultiply;
		case VK_SUBTRACT: return ImGuiKey_KeypadSubtract;
		case VK_ADD: return ImGuiKey_KeypadAdd;
		// Left Shift/Ctrl/Alt are submitted separately in SubmitInput() since the generic VK_SHIFT etc. alias them.
		case VK_RSHIFT: return ImGuiKey_RightShift;
		case VK_RCONTROL: return ImGuiKey_RightCtrl;
		case VK_RMENU: return ImGuiKey_RightAlt;
		case VK_LWIN: return ImGuiKey_LeftSuper;
		case VK_RWIN: return ImGuiKey_RightSuper;
		case VK_APPS: return ImGuiKey_Menu;
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

		for (int mouseBtn = 0; mouseBtn < 3; ++mouseBtn)
		{
			io.AddMouseButtonEvent(mouseBtn, mouseState.CurMouseBtnStates[mouseBtn]);
		}

		// CurWheelVal is the raw WM_MOUSEWHEEL delta for this frame (cleared next frame), ImGui wants notches.
		if (mouseState.CurWheelVal != 0)
		{
			io.AddMouseWheelEvent(0.0f, static_cast<float>(mouseState.CurWheelVal) / static_cast<float>(WHEEL_DELTA));
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

		const bool ctrlDown = keyboardState.IsDownThisFrame(VK_CONTROL) || keyboardState.IsDownThisFrame(VK_LCONTROL);
		const bool shiftDown = keyboardState.IsDownThisFrame(VK_SHIFT) || keyboardState.IsDownThisFrame(VK_LSHIFT);
		const bool altDown = keyboardState.IsDownThisFrame(VK_MENU) || keyboardState.IsDownThisFrame(VK_LMENU);
		io.AddKeyEvent(ImGuiKey_LeftCtrl, ctrlDown);
		io.AddKeyEvent(ImGuiKey_LeftShift, shiftDown);
		io.AddKeyEvent(ImGuiKey_LeftAlt, altDown);
		io.AddKeyEvent(ImGuiMod_Ctrl, ctrlDown || keyboardState.IsDownThisFrame(VK_RCONTROL));
		io.AddKeyEvent(ImGuiMod_Shift, shiftDown || keyboardState.IsDownThisFrame(VK_RSHIFT));
		io.AddKeyEvent(ImGuiMod_Alt, altDown || keyboardState.IsDownThisFrame(VK_RMENU));
	}
}
