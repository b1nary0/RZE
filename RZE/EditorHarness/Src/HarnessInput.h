#pragma once

#include <HarnessKeys.h>

#include <Utils/PrimitiveDefs.h>

#include <SDL3/SDL_events.h>

#include <deque>
#include <functional>
#include <string>
#include <vector>

struct SDL_Window;

namespace EditorHarness
{
	// Input for the Editor's window as SDL events, which go the way a real mouse and keyboard's do: through Window,
	// InputHandler and ImGui. An action plays out one step a frame (a click is a move, a press, then a release), as a
	// hand's would, and real mouse and keyboard input is dropped meanwhile, so nobody's hand on the mouse disturbs it.
	class HarnessInput
	{
	public:
		using Step = std::function<void()>;
		using Done = std::function<void()>;

		HarnessInput() = default;
		~HarnessInput();

		HarnessInput(const HarnessInput&) = delete;
		HarnessInput& operator=(const HarnessInput&) = delete;

		// Once the window exists
		void Initialize(SDL_Window* window);
		// Each frame, before the frame's commands: the next step of the playing action
		void Tick();

		// Played after any action before it; onDone once its last step has been pushed
		void Play(std::vector<Step>&& steps, Done&& onDone);

		// Real input is dropped by default
		void SetRealInputBlocked(bool isBlocked);
		bool IsRealInputBlocked() const { return m_isRealInputBlocked; }

		// Steps, each pushing what one frame of a hand would
		Step Move(float x, float y);
		Step Button(Uint8 button, bool isDown, float x, float y, Uint8 clicks);
		Step Wheel(float x, float y, float notches);
		Step Keys(const KeyStroke& stroke, bool isDown);
		// Modifier keys alone, held down or let go in reverse order, e.g. around a click
		Step Modifiers(const std::vector<SDL_Keycode>& modifiers, bool isDown);

	private:
		struct Action
		{
			std::vector<Step> Steps;
			size_t Next = 0;
			Done OnDone;
		};

		static bool SDLCALL Filter(void* self, SDL_Event* event);
		void Push(SDL_Event& event);
		// A key event, carrying the modifiers held
		void PushKey(SDL_Keycode key, bool isDown);
		// Each modifier pressed in order, or let go in reverse order, updating the modifiers held
		void PushModifiers(const std::vector<SDL_Keycode>& modifiers, bool isDown);
		// A copy SDL can point a text event at for as long as it's queued
		const char* KeepText(const std::string& text);

	private:
		SDL_Window* m_window = nullptr;
		SDL_WindowID m_windowID = 0;
		bool m_isRealInputBlocked = true;
		bool m_isPushing = false;
		SDL_MouseButtonFlags m_buttonsDown = 0;
		float m_mouseX = 0.0f;
		float m_mouseY = 0.0f;
		SDL_Keymod m_modifiers = SDL_KMOD_NONE;
		std::deque<Action> m_actions;
		U64 m_tick = 0;
		std::deque<std::pair<U64, std::string>> m_texts;
	};
}
