#include <UI/TitleBar.h>

#include <UI/Framework/EditorUI.h>

#include <RZE.h>

#include <Graphics/Texture2D.h>

#include <Utils/Platform/Filepath.h>

#include <Windowing/Win32Window.h>

#include <ImGui/imgui.h>

#include <algorithm>
#include <cmath>

namespace
{
	// The logo's height at 96 DPI. logo.png is twice this, so it's sharp up to 200% scaling: ImGui only samples a
	// texture's top mip, and a 2:1 bilinear minification still averages every pixel.
	constexpr float k_logoHeight = 24.0f;
	// logo.png ends at its divider, which is this much of its height past the letters. The menus start as far past the
	// divider, so it sits between them.
	constexpr float k_logoDividerGap = 1.0f / 3.0f;
	// Windows' caption buttons are 46 pixels wide at 96 DPI
	constexpr float k_buttonWidth = 46.0f;
	// Half the width of a button's glyph, at 96 DPI
	constexpr float k_glyphHalfSize = 5.0f;
	// Space kept around the title, at 96 DPI
	constexpr float k_titleMargin = 16.0f;

	const ImU32 k_buttonHovered = IM_COL32(255, 255, 255, 26);
	const ImU32 k_buttonPressed = IM_COL32(255, 255, 255, 46);
	// Windows' own close button colours
	const ImU32 k_closeHovered = IM_COL32(196, 43, 28, 255);
	const ImU32 k_closePressed = IM_COL32(200, 64, 49, 255);

	enum class EWindowButton
	{
		Minimize,
		Maximize,
		Restore,
		Close
	};

	// The bar's rect on screen
	ImVec2 GetBarMin()
	{
		return ImGui::GetWindowPos();
	}

	ImVec2 GetBarMax()
	{
		return ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowWidth(), ImGui::GetWindowPos().y + ImGui::GetWindowHeight());
	}

	// From screen to the window's client pixels, which is where the window's hit test looks
	Display::ScreenRect ToClientRect(const ImVec2& min, const ImVec2& max)
	{
		const ImVec2 origin = ImGui::GetMainViewport()->Pos;
		Display::ScreenRect rect;
		rect.Left = static_cast<int>(std::floor(min.x - origin.x));
		rect.Top = static_cast<int>(std::floor(min.y - origin.y));
		rect.Right = static_cast<int>(std::ceil(max.x - origin.x));
		rect.Bottom = static_cast<int>(std::ceil(max.y - origin.y));
		return rect;
	}

	void DrawGlyph(ImDrawList* drawList, EWindowButton button, const ImVec2& centre, float halfSize, float thickness, ImU32 colour)
	{
		// On pixel centres, so 1-pixel lines are crisp
		const ImVec2 c(std::floor(centre.x) + 0.5f, std::floor(centre.y) + 0.5f);
		const float s = std::floor(halfSize);
		switch (button)
		{
		case EWindowButton::Minimize:
			drawList->AddLine(ImVec2(c.x - s, c.y), ImVec2(c.x + s + 1.0f, c.y), colour, thickness);
			break;

		case EWindowButton::Maximize:
			drawList->AddRect(ImVec2(c.x - s, c.y - s), ImVec2(c.x + s, c.y + s), colour, 0.0f, 0, thickness);
			break;

		case EWindowButton::Restore:
		{
			// A window in front of another, offset up and to the right
			const float offset = std::floor(s * 0.4f);
			drawList->AddRect(ImVec2(c.x - s, c.y - s + offset), ImVec2(c.x + s - offset, c.y + s), colour, 0.0f, 0, thickness);
			drawList->AddLine(ImVec2(c.x - s + offset, c.y - s), ImVec2(c.x + s, c.y - s), colour, thickness);
			drawList->AddLine(ImVec2(c.x + s, c.y - s), ImVec2(c.x + s, c.y + s - offset), colour, thickness);
			break;
		}

		case EWindowButton::Close:
			drawList->AddLine(ImVec2(c.x - s, c.y - s), ImVec2(c.x + s, c.y + s), colour, thickness);
			drawList->AddLine(ImVec2(c.x + s, c.y - s), ImVec2(c.x - s, c.y + s), colour, thickness);
			break;
		}
	}

	// A button the bar's height, with its glyph drawn in the middle. True when it's clicked.
	bool WindowButton(const char* id, EWindowButton button, const ImVec2& min, const ImVec2& max, float dpiScale, bool isWindowFocused)
	{
		ImGui::SetCursorScreenPos(min);
		const bool isClicked = ImGui::InvisibleButton(id, ImVec2(max.x - min.x, max.y - min.y));
		const bool isHovered = ImGui::IsItemHovered();
		const bool isPressed = ImGui::IsItemActive();

		ImDrawList* const drawList = ImGui::GetWindowDrawList();
		const bool isClose = button == EWindowButton::Close;
		if (isPressed || isHovered)
		{
			const ImU32 background = isClose ? (isPressed ? k_closePressed : k_closeHovered) : (isPressed ? k_buttonPressed : k_buttonHovered);
			drawList->AddRectFilled(min, max, background);
		}

		const ImU32 colour = (isClose && isHovered) || isWindowFocused
			? ImGui::GetColorU32(ImGuiCol_Text)
			: ImGui::GetColorU32(ImGuiCol_TextDisabled);
		const ImVec2 centre((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
		DrawGlyph(drawList, button, centre, k_glyphHalfSize * dpiScale, std::max(1.0f, std::floor(dpiScale)), colour);
		return isClicked;
	}
}

namespace Editor
{
	void TitleBar::Initialize()
	{
		m_logo = RZE().GetResourceHandler().LoadResource<Texture2D>(Filepath("logo.png"));
	}

	void TitleBar::ShutDown()
	{
		m_logo = ResourceHandle::EmptyHandle();
	}

	void TitleBar::Begin(const Win32Window& window)
	{
		m_layout = TitleBarLayout();
		DrawLogo(window);
	}

	void TitleBar::AddControl(float left, float right)
	{
		m_layout.Controls.push_back(ToClientRect(ImVec2(left, GetBarMin().y), ImVec2(right, GetBarMax().y)));
	}

	void TitleBar::End(Win32Window& window)
	{
		const float buttonsLeft = GetBarMax().x - 3.0f * std::round(k_buttonWidth * window.GetDpiScale());
		DrawTitle(window, buttonsLeft);
		DrawWindowButtons(window);

		m_layout.Height = ToClientRect(GetBarMin(), GetBarMax()).Bottom;
		// Clicks on the bar close an open menu, rather than moving the window with the menu left open
		if (ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId))
		{
			m_layout.Controls.push_back(ToClientRect(GetBarMin(), GetBarMax()));
		}
		window.SetTitleBarLayout(std::move(m_layout));
	}

	void TitleBar::DrawLogo(const Win32Window& window)
	{
		const Texture2D* const logo = m_logo.IsValid() ? RZE().GetResourceHandler().GetResource<Texture2D>(m_logo) : nullptr;
		if (logo == nullptr || !logo->IsOnGPU())
		{
			return;
		}

		const ImVec2 barMin = GetBarMin();
		const float barHeight = GetBarMax().y - barMin.y;
		const Vector2D dimensions = logo->GetDimensions();
		const float height = std::min(barHeight, std::round(k_logoHeight * window.GetDpiScale()));
		const float width = std::round(height * dimensions.X() / dimensions.Y());

		// As far from the bar's left edge as from its top
		const float margin = std::round((barHeight - height) * 0.5f);
		const ImVec2 min(barMin.x + margin, barMin.y + margin);
		ImGui::GetWindowDrawList()->AddImage(EditorUI::TextureID(logo->GetPlatformObject()), min, ImVec2(min.x + width, min.y + height));

		// Moves the menus along past it. A menu in the bar puts its label half the item spacing past the cursor.
		const float menusLeft = min.x + width + std::round(height * k_logoDividerGap) - std::floor(ImGui::GetStyle().ItemSpacing.x * 0.5f);
		ImGui::SetCursorScreenPos(ImVec2(menusLeft, ImGui::GetCursorScreenPos().y));
	}

	void TitleBar::DrawTitle(const Win32Window& window, float right)
	{
		const std::string& title = window.GetTitle();
		const float margin = k_titleMargin * window.GetDpiScale();
		const float left = ImGui::GetCursorScreenPos().x + margin;
		right -= margin;
		if (title.empty() || right <= left)
		{
			return;
		}

		// Centred on the bar, unless that would run into what's beside it
		const ImVec2 barMin = GetBarMin();
		const ImVec2 barMax = GetBarMax();
		const ImVec2 size = ImGui::CalcTextSize(title.c_str());
		float x = (barMin.x + barMax.x - size.x) * 0.5f;
		x = std::max(left, std::min(x, right - size.x));
		const ImVec2 position(std::round(x), std::round((barMin.y + barMax.y - size.y) * 0.5f));

		const ImVec4 clip(left, barMin.y, right, barMax.y);
		const ImU32 colour = ImGui::GetColorU32(window.IsFocused() ? ImGuiCol_Text : ImGuiCol_TextDisabled);
		ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), ImGui::GetFontSize(), position, colour, title.c_str(), nullptr, 0.0f, &clip);
	}

	void TitleBar::DrawWindowButtons(Win32Window& window)
	{
		const float dpiScale = window.GetDpiScale();
		const float width = std::round(k_buttonWidth * dpiScale);
		const ImVec2 barMax = GetBarMax();
		const float top = GetBarMin().y;
		const bool isFocused = window.IsFocused();
		const bool isMaximized = window.IsMaximised();

		const ImVec2 minimizeMin(barMax.x - 3.0f * width, top);
		const ImVec2 maximizeMin(barMax.x - 2.0f * width, top);
		const ImVec2 closeMin(barMax.x - width, top);

		if (WindowButton("##Minimize", EWindowButton::Minimize, minimizeMin, ImVec2(maximizeMin.x, barMax.y), dpiScale, isFocused))
		{
			window.Minimize();
		}
		if (WindowButton("##Maximize", isMaximized ? EWindowButton::Restore : EWindowButton::Maximize, maximizeMin, ImVec2(closeMin.x, barMax.y), dpiScale, isFocused))
		{
			if (isMaximized)
			{
				window.Restore();
			}
			else
			{
				window.Maximize();
			}
		}
		if (WindowButton("##Close", EWindowButton::Close, closeMin, barMax, dpiScale, isFocused))
		{
			window.RequestClose();
		}

		m_layout.Controls.push_back(ToClientRect(minimizeMin, ImVec2(maximizeMin.x, barMax.y)));
		m_layout.Controls.push_back(ToClientRect(closeMin, barMax));
		m_layout.MaximizeButton = ToClientRect(maximizeMin, ImVec2(closeMin.x, barMax.y));
	}
}
