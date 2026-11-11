#include <HarnessInput.h>

#include <SDL3/SDL.h>

namespace EditorHarness
{
	namespace
	{
		// Frames a pushed text event's string is kept: it's read on the next frame's pump
		constexpr U64 k_textFrames = 8;

		SDL_Keymod ModifierFlag(SDL_Keycode key)
		{
			switch (key)
			{
			case SDLK_LCTRL: return SDL_KMOD_LCTRL;
			case SDLK_LSHIFT: return SDL_KMOD_LSHIFT;
			case SDLK_LALT: return SDL_KMOD_LALT;
			default: return SDL_KMOD_NONE;
			}
		}
	}

	HarnessInput::~HarnessInput()
	{
		if (m_window != nullptr)
		{
			SDL_SetEventFilter(nullptr, nullptr);
		}
	}

	void HarnessInput::Initialize(SDL_Window* window)
	{
		m_window = window;
		m_windowID = SDL_GetWindowID(window);
		// Called for every event as it's queued, real ones as the OS's are pumped and the harness's inside Push
		SDL_SetEventFilter(&HarnessInput::Filter, this);
	}

	void HarnessInput::Tick()
	{
		++m_tick;
		while (!m_texts.empty() && m_texts.front().first + k_textFrames < m_tick)
		{
			m_texts.pop_front();
		}

		if (m_actions.empty())
		{
			return;
		}
		Action& action = m_actions.front();
		if (action.Next < action.Steps.size())
		{
			action.Steps[action.Next++]();
		}
		if (action.Next == action.Steps.size())
		{
			Done onDone = std::move(action.OnDone);
			m_actions.pop_front();
			if (onDone)
			{
				onDone();
			}
		}
	}

	void HarnessInput::Play(std::vector<Step>&& steps, Done&& onDone)
	{
		Action action;
		action.Steps = std::move(steps);
		// Two frames more before it's done: events pushed on one frame are pumped at the start of the next, and the
		// UI acts on them during that one
		action.Steps.push_back([]() {});
		action.Steps.push_back([]() {});
		action.OnDone = std::move(onDone);
		m_actions.push_back(std::move(action));
	}

	void HarnessInput::SetRealInputBlocked(bool isBlocked)
	{
		const bool isUnblocking = m_isRealInputBlocked && !isBlocked;
		m_isRealInputBlocked = isBlocked;

		// While blocked, the real mouse's moves were dropped, so the UI still has the mouse where the harness last put
		// it: a real click before any real move would land there. SDL tracks the real mouse before the filter runs, so
		// when it's over the window, a move to it is pushed, as a hand's nudge would send.
		if (isUnblocking && m_window != nullptr && SDL_GetMouseFocus() == m_window)
		{
			float x = 0.0f;
			float y = 0.0f;
			SDL_GetMouseState(&x, &y);
			Move(x, y)();
		}
	}

	HarnessInput::Step HarnessInput::Move(float x, float y)
	{
		return [this, x, y]()
			{
				SDL_Event event = {};
				event.type = SDL_EVENT_MOUSE_MOTION;
				event.motion.windowID = m_windowID;
				event.motion.state = m_buttonsDown;
				event.motion.x = x;
				event.motion.y = y;
				event.motion.xrel = x - m_mouseX;
				event.motion.yrel = y - m_mouseY;
				m_mouseX = x;
				m_mouseY = y;
				Push(event);
			};
	}

	HarnessInput::Step HarnessInput::Button(Uint8 button, bool isDown, float x, float y, Uint8 clicks)
	{
		return [this, button, isDown, x, y, clicks]()
			{
				const SDL_MouseButtonFlags mask = SDL_BUTTON_MASK(button);
				m_buttonsDown = isDown ? (m_buttonsDown | mask) : (m_buttonsDown & ~mask);

				SDL_Event event = {};
				event.type = isDown ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
				event.button.windowID = m_windowID;
				event.button.button = button;
				event.button.down = isDown;
				event.button.clicks = clicks;
				event.button.x = x;
				event.button.y = y;
				Push(event);
			};
	}

	HarnessInput::Step HarnessInput::Wheel(float x, float y, float notches)
	{
		return [this, x, y, notches]()
			{
				SDL_Event event = {};
				event.type = SDL_EVENT_MOUSE_WHEEL;
				event.wheel.windowID = m_windowID;
				event.wheel.y = notches;
				event.wheel.direction = SDL_MOUSEWHEEL_NORMAL;
				event.wheel.mouse_x = x;
				event.wheel.mouse_y = y;
				Push(event);
			};
	}

	HarnessInput::Step HarnessInput::Keys(const KeyStroke& stroke, bool isDown)
	{
		return [this, stroke, isDown]()
			{
				const auto pushKey = [this](SDL_Keycode key, bool down)
					{
						SDL_Event event = {};
						event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
						event.key.windowID = m_windowID;
						event.key.key = key;
						event.key.scancode = SDL_GetScancodeFromKey(key, nullptr);
						event.key.mod = m_modifiers;
						event.key.down = down;
						Push(event);
					};

				if (isDown)
				{
					// Modifiers first, as a hand holds them, then the key, and what it types
					for (const SDL_Keycode modifier : stroke.Modifiers)
					{
						m_modifiers = static_cast<SDL_Keymod>(m_modifiers | ModifierFlag(modifier));
						pushKey(modifier, true);
					}
					pushKey(stroke.Key, true);
					if (!stroke.Text.empty())
					{
						SDL_Event event = {};
						event.type = SDL_EVENT_TEXT_INPUT;
						event.text.windowID = m_windowID;
						event.text.text = KeepText(stroke.Text);
						Push(event);
					}
				}
				else
				{
					pushKey(stroke.Key, false);
					for (auto modifier = stroke.Modifiers.rbegin(); modifier != stroke.Modifiers.rend(); ++modifier)
					{
						m_modifiers = static_cast<SDL_Keymod>(m_modifiers & ~ModifierFlag(*modifier));
						pushKey(*modifier, false);
					}
				}
			};
	}

	bool SDLCALL HarnessInput::Filter(void* self, SDL_Event* event)
	{
		const HarnessInput& input = *static_cast<const HarnessInput*>(self);
		if (!input.m_isRealInputBlocked || input.m_isPushing)
		{
			return true;
		}

		switch (event->type)
		{
		case SDL_EVENT_MOUSE_MOTION:
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP:
		case SDL_EVENT_MOUSE_WHEEL:
		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP:
		case SDL_EVENT_TEXT_INPUT:
		case SDL_EVENT_TEXT_EDITING:
			return false;
		default:
			return true;
		}
	}

	void HarnessInput::Push(SDL_Event& event)
	{
		// The filter lets through only what's pushed here
		m_isPushing = true;
		SDL_PushEvent(&event);
		m_isPushing = false;
	}

	const char* HarnessInput::KeepText(const std::string& text)
	{
		m_texts.emplace_back(m_tick, text);
		return m_texts.back().second.c_str();
	}
}
