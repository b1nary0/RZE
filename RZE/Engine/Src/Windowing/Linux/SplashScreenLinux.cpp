#include <StdAfx.h>

#include <Windowing/SplashScreen.h>

#include <Windowing/Display.h>
#include <Windowing/Linux/WindowIconLinux.h>
#include <Windowing/SDLVideo.h>
#include <Windowing/SplashScreenLayout.h>
#include <Windowing/WindowIcon.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Platform/Filepath.h>

// Its own copy: ImGui's is private to imgui_draw.cpp
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include <imgui/imstb_truetype.h>

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <mutex>
#include <string>
#include <vector>

// SDL only makes and draws windows on the main thread, so this splash screen is drawn there: when it's made, and each
// time the main thread sets the status. It looks as it does on Windows (SplashScreenLayout.h), drawn into a BGRA canvas
// and copied to the window's surface, with no GPU involved.
namespace
{
	using namespace SplashScreenLayout;

	// The Editor's UI font, for the status line in place of Windows' Segoe UI
	constexpr const char* k_fontPath = "Assets/Fonts/Ubuntu-Regular.ttf";
	constexpr const char* k_ellipsis = "...";

	// The status text's height, as Windows' CreateFont takes a negative point size: the font's em, in pixels at 96 DPI
	constexpr float k_statusEmPixels = k_statusPointSize * 96.0f / 72.0f;

	// The next code point in text from index, which it moves past; '?' for bytes that aren't UTF-8
	int NextCodepoint(const std::string& text, size_t& index)
	{
		const unsigned char lead = static_cast<unsigned char>(text[index++]);
		const int extraBytes = lead < 0x80 ? 0 : (lead >> 5) == 0x6 ? 1 : (lead >> 4) == 0xE ? 2 : (lead >> 3) == 0x1E ? 3 : -1;
		if (extraBytes < 0)
		{
			return '?';
		}

		int codepoint = extraBytes == 0 ? lead : lead & (0x3F >> extraBytes);
		for (int i = 0; i < extraBytes && index < text.size(); ++i)
		{
			codepoint = (codepoint << 6) | (static_cast<unsigned char>(text[index++]) & 0x3F);
		}
		return codepoint;
	}

	void FillRect(std::vector<U8>& canvas, int canvasWidth, int canvasHeight, int left, int top, int right, int bottom, const Colour& colour)
	{
		for (int y = std::max(0, top); y < std::min(bottom, canvasHeight); ++y)
		{
			for (int x = std::max(0, left); x < std::min(right, canvasWidth); ++x)
			{
				U8* const texel = &canvas[(static_cast<size_t>(y) * canvasWidth + x) * k_bytesPerTexel];
				texel[0] = colour.B;
				texel[1] = colour.G;
				texel[2] = colour.R;
			}
		}
	}
}

struct SplashScreen::State
{
	// Draws the canvas with the status as it is now, and puts it on the window. Main thread only.
	void Draw();
	// text's width in pixels, as DrawText would draw it
	float MeasureText(const std::string& text) const;
	// text with its bottom at bottom, starting at left: what fits in the width left to right, ending in an ellipsis if
	// it's cut short, as Windows' DT_END_ELLIPSIS does
	void DrawStatus(std::vector<U8>& canvas, std::string text, int left, int right, int bottom) const;

	// Keeps SDL video running while the splash screen is up
	SDLVideo::Scope Video;
	SDL_Window* Window = nullptr;
	SDL_Cursor* BusyCursor = nullptr;

	// The background with the image on it
	std::vector<U8> Background;

	std::vector<U8> FontData;
	stbtt_fontinfo Font = {};
	bool HasFont = false;
	float FontScale = 1.0f;

	std::mutex StatusMutex;
	std::string Status;
	// k_noProgress for no progress bar
	float Progress = k_noProgress;
};

float SplashScreen::State::MeasureText(const std::string& text) const
{
	float width = 0.0f;
	int previous = 0;
	for (size_t index = 0; index < text.size();)
	{
		const int codepoint = NextCodepoint(text, index);
		int advance = 0;
		int leftBearing = 0;
		stbtt_GetCodepointHMetrics(&Font, codepoint, &advance, &leftBearing);
		width += (advance + (previous != 0 ? stbtt_GetCodepointKernAdvance(&Font, previous, codepoint) : 0)) * FontScale;
		previous = codepoint;
	}
	return width;
}

void SplashScreen::State::DrawStatus(std::vector<U8>& canvas, std::string text, int left, int right, int bottom) const
{
	if (!HasFont || text.empty())
	{
		return;
	}

	const float available = static_cast<float>(right - left);
	if (MeasureText(text) > available)
	{
		// Whole code points off the end until it fits with the ellipsis
		while (!text.empty() && MeasureText(text + k_ellipsis) > available)
		{
			text.pop_back();
			while (!text.empty() && (static_cast<unsigned char>(text.back()) & 0xC0) == 0x80)
			{
				text.pop_back();
			}
		}
		text += k_ellipsis;
	}

	int ascent = 0;
	int descent = 0;
	int lineGap = 0;
	stbtt_GetFontVMetrics(&Font, &ascent, &descent, &lineGap);
	// The line's bottom (its descent, which is negative) on bottom
	const int baseline = bottom + static_cast<int>(std::lround(descent * FontScale));

	float penX = static_cast<float>(left);
	int previous = 0;
	std::vector<U8> coverage;
	for (size_t index = 0; index < text.size();)
	{
		const int codepoint = NextCodepoint(text, index);
		if (previous != 0)
		{
			penX += stbtt_GetCodepointKernAdvance(&Font, previous, codepoint) * FontScale;
		}

		int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
		stbtt_GetCodepointBitmapBox(&Font, codepoint, FontScale, FontScale, &x0, &y0, &x1, &y1);
		const int glyphWidth = x1 - x0;
		const int glyphHeight = y1 - y0;
		if (glyphWidth > 0 && glyphHeight > 0)
		{
			coverage.assign(static_cast<size_t>(glyphWidth) * glyphHeight, 0);
			stbtt_MakeCodepointBitmap(&Font, coverage.data(), glyphWidth, glyphHeight, glyphWidth, FontScale, FontScale, codepoint);

			const int glyphLeft = static_cast<int>(std::lround(penX)) + x0;
			const int glyphTop = baseline + y0;
			for (int y = 0; y < glyphHeight; ++y)
			{
				const int canvasY = glyphTop + y;
				if (canvasY < 0 || canvasY >= k_height)
				{
					continue;
				}
				for (int x = 0; x < glyphWidth; ++x)
				{
					const int canvasX = glyphLeft + x;
					if (canvasX < 0 || canvasX >= k_width)
					{
						continue;
					}
					const float alpha = coverage[static_cast<size_t>(y) * glyphWidth + x] / 255.0f;
					U8* const texel = &canvas[(static_cast<size_t>(canvasY) * k_width + canvasX) * k_bytesPerTexel];
					texel[0] = static_cast<U8>(std::lround(texel[0] + (k_statusColour.B - texel[0]) * alpha));
					texel[1] = static_cast<U8>(std::lround(texel[1] + (k_statusColour.G - texel[1]) * alpha));
					texel[2] = static_cast<U8>(std::lround(texel[2] + (k_statusColour.R - texel[2]) * alpha));
				}
			}
		}

		int advance = 0;
		int leftBearing = 0;
		stbtt_GetCodepointHMetrics(&Font, codepoint, &advance, &leftBearing);
		penX += advance * FontScale;
		previous = codepoint;
	}
}

void SplashScreen::State::Draw()
{
	std::string status;
	float progress = k_noProgress;
	{
		std::lock_guard lock(StatusMutex);
		status = Status;
		progress = Progress;
	}

	// The same layout as the Windows splash screen's Paint, at 96 DPI
	std::vector<U8> canvas = Background;
	const int barTop = k_height - k_statusMargin - k_progressBarHeight;
	DrawStatus(canvas, status, k_statusMargin, k_width - k_statusMargin, barTop - k_progressBarGap);
	if (progress >= 0.0f)
	{
		FillRect(canvas, k_width, k_height, k_statusMargin, barTop, k_width - k_statusMargin, barTop + k_progressBarHeight, k_progressTrackColour);
		const int barRight = k_statusMargin + static_cast<int>(std::lround((k_width - 2 * k_statusMargin) * std::min(progress, 1.0f)));
		FillRect(canvas, k_width, k_height, k_statusMargin, barTop, barRight, barTop + k_progressBarHeight, k_progressColour);
	}

	SDL_Surface* const windowSurface = SDL_GetWindowSurface(Window);
	SDL_Surface* const canvasSurface = SDL_CreateSurfaceFrom(k_width, k_height, SDL_PIXELFORMAT_BGRA32, canvas.data(), k_width * k_bytesPerTexel);
	if (windowSurface != nullptr && canvasSurface != nullptr)
	{
		SDL_BlitSurface(canvasSurface, nullptr, windowSurface, nullptr);
		SDL_UpdateWindowSurface(Window);
	}
	SDL_DestroySurface(canvasSurface);

	// The desktop sees it answer; the window's events are left for the main window, which ignores them
	SDL_PumpEvents();
}

SplashScreen::SplashScreen(const Filepath& image, const std::string& title, int monitorIndex)
	: m_state(std::make_unique<State>())
{
	State& state = *m_state;
	if (!state.Video.IsAcquired())
	{
		return;
	}

	state.Background.resize(static_cast<size_t>(k_width) * k_height * k_bytesPerTexel);
	for (size_t texel = 0; texel < state.Background.size(); texel += k_bytesPerTexel)
	{
		state.Background[texel + 0] = k_backgroundColour.B;
		state.Background[texel + 1] = k_backgroundColour.G;
		state.Background[texel + 2] = k_backgroundColour.R;
		state.Background[texel + 3] = 255;
	}

	const IconImage splashImage = WindowIcon::ReadImage(image);
	if (splashImage.IsEmpty())
	{
		RZE_LOG_WARNING_ARGS("Couldn't read the splash screen image [%s].", image.GetRelativePath().c_str());
	}
	else
	{
		DrawImage(splashImage, (k_width - splashImage.Width) / 2, (k_height - splashImage.Height) / 2, splashImage.Width, splashImage.Height, state.Background, k_width, k_height);
	}

	std::ifstream fontFile(Filepath(k_fontPath).GetAbsolutePath(), std::ios::binary);
	state.FontData.assign(std::istreambuf_iterator<char>(fontFile), std::istreambuf_iterator<char>());
	state.HasFont = !state.FontData.empty() && stbtt_InitFont(&state.Font, state.FontData.data(), stbtt_GetFontOffsetForIndex(state.FontData.data(), 0)) != 0;
	if (state.HasFont)
	{
		state.FontScale = stbtt_ScaleForMappingEmToPixels(&state.Font, k_statusEmPixels);
	}
	else
	{
		RZE_LOG_WARNING_ARGS("Couldn't read the splash screen's font [%s]; the status won't show.", k_fontPath);
	}

	// No frame, and no taskbar entry where the desktop allows it: the main window's is the application's
	state.Window = SDL_CreateWindow(title.c_str(), k_width, k_height, SDL_WINDOW_BORDERLESS | SDL_WINDOW_UTILITY | SDL_WINDOW_HIDDEN);
	if (state.Window == nullptr)
	{
		RZE_LOG_WARNING("Couldn't make the splash screen's window: " << SDL_GetError());
		return;
	}
	// For wherever the desktop lists it anyway (a taskbar, Alt+Tab)
	WindowIconLinux::SetDefault(state.Window);

	// Centred on the monitor's work area, where the desktop lets an application place its windows (X11, not Wayland)
	Display::MonitorInfo monitorInfo;
	if (!Display::GetMonitor(monitorIndex, monitorInfo))
	{
		Display::GetMonitor(0, monitorInfo);
	}
	const Display::ScreenRect& workArea = monitorInfo.WorkArea;
	SDL_SetWindowPosition(state.Window, workArea.Left + (workArea.Width() - k_width) / 2, workArea.Top + (workArea.Height() - k_height) / 2);

	// As the Windows splash screen's class cursor: busy, but still pointing
	state.BusyCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_PROGRESS);
	SDL_SetCursor(state.BusyCursor);

	SDL_ShowWindow(state.Window);
	// Drawn now, so it's on screen before the constructor returns
	state.Draw();
}

SplashScreen::~SplashScreen()
{
	State& state = *m_state;
	if (state.Window != nullptr)
	{
		SDL_DestroyWindow(state.Window);
	}
	if (state.BusyCursor != nullptr)
	{
		SDL_SetCursor(SDL_GetDefaultCursor());
		SDL_DestroyCursor(state.BusyCursor);
	}
}

void SplashScreen::SetStatus(const std::string& status)
{
	SetStatus(status, SplashScreenLayout::k_noProgress);
}

void SplashScreen::SetStatus(const std::string& status, float progress)
{
	{
		std::lock_guard lock(m_state->StatusMutex);
		// Loading sets it every pass, mostly to what it already is
		if (status == m_state->Status && progress == m_state->Progress)
		{
			return;
		}
		m_state->Status = status;
		m_state->Progress = progress;
	}

	// Only the main thread can draw it; from another thread, the next status the main thread sets shows this one too
	if (m_state->Window != nullptr && SDL_IsMainThread())
	{
		m_state->Draw();
	}
}
