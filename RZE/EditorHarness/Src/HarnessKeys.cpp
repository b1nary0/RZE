#include <HarnessKeys.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <map>

namespace EditorHarness
{
	namespace
	{
		const std::map<std::string, SDL_Keycode>& NamedKeys()
		{
			static const std::map<std::string, SDL_Keycode> s_keys = {
				{ "ENTER", SDLK_RETURN }, { "TAB", SDLK_TAB }, { "ESC", SDLK_ESCAPE }, { "BACKSPACE", SDLK_BACKSPACE },
				{ "BS", SDLK_BACKSPACE }, { "SPACE", SDLK_SPACE }, { "DELETE", SDLK_DELETE }, { "DEL", SDLK_DELETE },
				{ "INSERT", SDLK_INSERT }, { "INS", SDLK_INSERT }, { "HOME", SDLK_HOME }, { "END", SDLK_END },
				{ "PGUP", SDLK_PAGEUP }, { "PGDN", SDLK_PAGEDOWN }, { "UP", SDLK_UP }, { "DOWN", SDLK_DOWN },
				{ "LEFT", SDLK_LEFT }, { "RIGHT", SDLK_RIGHT }, { "PRTSC", SDLK_PRINTSCREEN }, { "PAUSE", SDLK_PAUSE },
				{ "F1", SDLK_F1 }, { "F2", SDLK_F2 }, { "F3", SDLK_F3 }, { "F4", SDLK_F4 }, { "F5", SDLK_F5 },
				{ "F6", SDLK_F6 }, { "F7", SDLK_F7 }, { "F8", SDLK_F8 }, { "F9", SDLK_F9 }, { "F10", SDLK_F10 },
				{ "F11", SDLK_F11 }, { "F12", SDLK_F12 },
			};
			return s_keys;
		}

		// The key a US layout types character on, and whether it needs Shift. Unshifted keys' codes are their
		// characters (SDLK_A is 'a').
		bool FindCharacterKey(char character, SDL_Keycode& outKey, bool& outShift)
		{
			static const char* const k_shifted = "!@#$%^&*()_+{}:\"~|<>?";
			static const char* const k_unshifted = "1234567890-=[];'`\\,./";
			if (std::isalpha(static_cast<unsigned char>(character)))
			{
				outKey = static_cast<SDL_Keycode>(std::tolower(static_cast<unsigned char>(character)));
				outShift = std::isupper(static_cast<unsigned char>(character)) != 0;
				return true;
			}
			if (const char* shifted = std::strchr(k_shifted, character); shifted != nullptr && character != '\0')
			{
				outKey = static_cast<SDL_Keycode>(k_unshifted[shifted - k_shifted]);
				outShift = true;
				return true;
			}
			if (std::isdigit(static_cast<unsigned char>(character)) || std::strchr(k_unshifted, character) != nullptr || character == ' ')
			{
				outKey = static_cast<SDL_Keycode>(character);
				outShift = false;
				return character != '\0';
			}
			return false;
		}

		// What a character's key types with Shift held, on a US layout
		std::string ShiftedText(const std::string& text)
		{
			static const char* const k_shifted = "!@#$%^&*()_+{}:\"~|<>?";
			static const char* const k_unshifted = "1234567890-=[];'`\\,./";
			if (text.size() != 1)
			{
				return text;
			}
			const char character = text[0];
			if (std::isalpha(static_cast<unsigned char>(character)))
			{
				return std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(character))));
			}
			if (const char* unshifted = std::strchr(k_unshifted, character); unshifted != nullptr)
			{
				return std::string(1, k_shifted[unshifted - k_unshifted]);
			}
			return text;
		}

		SDL_Keycode ModifierKey(char modifier)
		{
			switch (modifier)
			{
			case '^': return SDLK_LCTRL;
			case '+': return SDLK_LSHIFT;
			case '%': return SDLK_LALT;
			default: return SDLK_UNKNOWN;
			}
		}
	}

	bool ParseKeys(const std::string& keys, std::vector<KeyStroke>& outStrokes, std::string& outError)
	{
		std::vector<SDL_Keycode> held;
		std::vector<SDL_Keycode> group;
		bool isInGroup = false;
		size_t i = 0;
		while (i < keys.size())
		{
			const char character = keys[i];
			if (ModifierKey(character) != SDLK_UNKNOWN)
			{
				held.push_back(ModifierKey(character));
				++i;
				continue;
			}
			if (character == '(' && !held.empty())
			{
				group = held;
				held.clear();
				isInGroup = true;
				++i;
				continue;
			}
			if (character == ')' && isInGroup)
			{
				group.clear();
				isInGroup = false;
				++i;
				continue;
			}

			SDL_Keycode key = SDLK_UNKNOWN;
			bool needsShift = false;
			std::string text;
			int count = 1;
			if (character == '{')
			{
				// "{}}" is a literal }, so the closing brace is looked for after it
				const size_t end = keys.find('}', keys.compare(i, 3, "{}}") == 0 ? i + 2 : i + 1);
				if (end == std::string::npos)
				{
					outError = "Unclosed { in keys: " + keys;
					return false;
				}
				const std::string body = keys.substr(i + 1, end - i - 1);
				const size_t space = body.find(' ');
				std::string name = body.substr(0, space);
				if (space != std::string::npos)
				{
					count = std::atoi(body.c_str() + space + 1);
				}
				std::string upper = name;
				for (char& c : upper)
				{
					c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
				}
				if (NamedKeys().count(upper) != 0)
				{
					key = NamedKeys().at(upper);
					text = key == SDLK_SPACE ? " " : "";
				}
				else if (name.size() == 1 && FindCharacterKey(name[0], key, needsShift))
				{
					text = name;
				}
				else
				{
					outError = "Unknown key {" + body + "}";
					return false;
				}
				i = end + 1;
			}
			else
			{
				if (character == '~')
				{
					key = SDLK_RETURN;
				}
				else if (FindCharacterKey(character, key, needsShift))
				{
					text = std::string(1, character);
				}
				else
				{
					outError = std::string("Can't type '") + character + "' on a US layout";
					return false;
				}
				++i;
			}

			KeyStroke stroke;
			stroke.Modifiers = group;
			stroke.Modifiers.insert(stroke.Modifiers.end(), held.begin(), held.end());
			const bool isShortcut = std::find_if(stroke.Modifiers.begin(), stroke.Modifiers.end(),
				[](SDL_Keycode modifier) { return modifier == SDLK_LCTRL || modifier == SDLK_LALT; }) != stroke.Modifiers.end();
			if (needsShift && std::find(stroke.Modifiers.begin(), stroke.Modifiers.end(), SDLK_LSHIFT) == stroke.Modifiers.end())
			{
				stroke.Modifiers.push_back(SDLK_LSHIFT);
			}
			const bool isShifted = std::find(stroke.Modifiers.begin(), stroke.Modifiers.end(), SDLK_LSHIFT) != stroke.Modifiers.end();
			stroke.Key = key;
			// Ctrl and Alt make a key a shortcut, which types nothing
			stroke.Text = isShortcut ? std::string() : (isShifted && !needsShift ? ShiftedText(text) : text);
			for (int n = 0; n < count; ++n)
			{
				outStrokes.push_back(stroke);
			}
			held.clear();
		}
		return true;
	}

	SDL_Keycode ModifierKeyNamed(const std::string& name)
	{
		if (name == "ctrl")
		{
			return SDLK_LCTRL;
		}
		if (name == "shift")
		{
			return SDLK_LSHIFT;
		}
		return name == "alt" ? SDLK_LALT : SDLK_UNKNOWN;
	}
}
