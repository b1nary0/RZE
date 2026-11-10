#pragma once

#include <SDL3/SDL_keycode.h>

#include <string>
#include <vector>

namespace EditorHarness
{
	// One key as a hand presses it: its modifiers held around it, and the text it types, if any
	struct KeyStroke
	{
		std::vector<SDL_Keycode> Modifiers;
		SDL_Keycode Key = SDLK_UNKNOWN;
		// What a US layout types for it ("A" for Shift+a); empty with Ctrl or Alt held, or for keys that type nothing
		std::string Text;
	};

	// SendKeys syntax, as the run-rze drivers take it: ^ Ctrl, + Shift, % Alt (on the next key, or a (group)),
	// {NAME} or {NAME n} for named keys, {+} {^} {%} {~} {(} {)} {{} {}} for those characters, ~ for Enter. A US
	// layout's characters. False, with outError saying why, if keys can't be parsed.
	bool ParseKeys(const std::string& keys, std::vector<KeyStroke>& outStrokes, std::string& outError);

	// A modifier key by the drivers' name for it, as their pointer commands' "mods" take it: "ctrl", "shift" or "alt".
	// SDLK_UNKNOWN for any other name.
	SDL_Keycode ModifierKeyNamed(const std::string& name);
}
