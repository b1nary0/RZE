#include <UI/MainMenuBar.h>

#include <UI/Framework/EditorContext.h>
#include <UI/Framework/EditorUI.h>
#include <UI/Modals/UnsavedChangesPrompt.h>

#include <Commands/HistoryCommands.h>
#include <Commands/SceneCommands.h>

#include <Services/Thumbnails.h>

#include <RZE.h>

#include <Asset/AssetPaths.h>

#include <Game/GameScene.h>

#include <Utils/Platform/Filepath.h>

#include <Windowing/Win32Window.h>

#include <ImGui/imgui.h>
#include <imGUI/ImGuizmo.h>

#include <algorithm>
#include <cmath>

namespace
{
	// Undo, redo and saving wait while a widget or the gizmo is held, so a drag isn't half undone and then carried on,
	// or saved halfway. A text field being edited also handles Ctrl+Z itself.
	bool IsEditInProgress()
	{
		return ImGui::IsAnyItemActive() || ImGuizmo::IsUsing();
	}

	// The bar's height at 96 DPI: taller than the OS's title bar, to give the logo room
	constexpr float k_barHeight = 40.0f;
	// A menu's highlight, at 96 DPI: kept this far inside its header at the sides, so neighbours' highlights don't run
	// together, and this far from the bar's top and bottom; and outlined this thick, so its edge is clear against the bar
	constexpr float k_highlightInsetX = 1.0f;
	constexpr float k_highlightInsetY = 4.0f;
	constexpr float k_highlightBorder = 1.0f;
	// Darker than the bar, so it edges the highlight without standing out
	const ImU32 k_highlightBorderColour = IM_COL32(24, 24, 24, 255);

	// The bar's draw list channels: a menu's highlight is drawn once ImGui has drawn its label, but goes behind it
	constexpr int k_highlightChannel = 0;
	constexpr int k_contentChannel = 1;

	// What the bar's menus need to draw their highlights
	struct BarMenuStyle
	{
		ImDrawList* DrawList{ nullptr };
		// The bar's
		float Top{ 0.0f };
		float Bottom{ 0.0f };
		// Above and below the menus' labels
		float Padding{ 0.0f };
		// k_highlightInsetX, k_highlightInsetY, k_highlightBorder and EditorUI::k_cornerRounding at the window's DPI
		float InsetX{ 0.0f };
		float InsetY{ 0.0f };
		float Border{ 0.0f };
		float Rounding{ 0.0f };
	};

	// A rounded rectangle in the highlight colour, outlined
	void DrawMenuHighlight(const BarMenuStyle& style, float left, float right, ImU32 colour)
	{
		// On whole pixels, so the outline is crisp
		const ImVec2 min(std::round(left + style.InsetX), std::round(style.Top + style.InsetY));
		const ImVec2 max(std::round(right - style.InsetX), std::round(style.Bottom - style.InsetY));

		ImDrawList& drawList = *style.DrawList;
		drawList.ChannelsSetCurrent(k_highlightChannel);
		drawList.AddRectFilled(min, max, colour, style.Rounding);
		drawList.AddRect(min, max, k_highlightBorderColour, style.Rounding, 0, style.Border);
		drawList.ChannelsSetCurrent(k_contentChannel);
	}

	// BeginMenu for a menu in the bar, hovered anywhere up and down the bar, and highlighted while it's hovered or open
	bool BeginBarMenu(const BarMenuStyle& style, const char* label)
	{
		// A header's box reaches half this above and below its label, so it fills the bar, for hovering too. Popped
		// before the menu's items, which keep their own spacing.
		ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, style.Padding * 2.0f);
		// In place of ImGui's own highlight, which only covers the box
		ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32_BLACK_TRANS);
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32_BLACK_TRANS);
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32_BLACK_TRANS);
		const bool isOpen = ImGui::BeginMenu(label);
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar();

		// BeginMenu leaves the header as the last item, even with its menu open
		const bool isHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
		if (isOpen || isHovered)
		{
			DrawMenuHighlight(style, ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().x, ImGui::GetColorU32(isHovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header));
		}
		return isOpen;
	}
}

namespace Editor
{
	void MainMenuBar::Initialize()
	{
		m_titleBar.Initialize();
	}

	void MainMenuBar::ShutDown()
	{
		m_titleBar.ShutDown();
	}

	void MainMenuBar::Draw(EditorContext& context, Win32Window& window)
	{
		HandleShortcuts(context);

		// The bar is its font plus this above and below. Never shorter than the OS's title bar.
		const float height = std::max(static_cast<float>(window.GetSystemTitleBarHeight()), std::round(k_barHeight * window.GetDpiScale()));
		const float padding = std::max(0.0f, (height - ImGui::GetFontSize()) * 0.5f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.5f, padding));
		if (ImGui::BeginMainMenuBar())
		{
			BarMenuStyle menuStyle;
			menuStyle.DrawList = ImGui::GetWindowDrawList();
			menuStyle.Top = ImGui::GetWindowPos().y;
			menuStyle.Bottom = menuStyle.Top + ImGui::GetWindowHeight();
			menuStyle.Padding = padding;
			menuStyle.InsetX = std::round(k_highlightInsetX * window.GetDpiScale());
			menuStyle.InsetY = std::round(k_highlightInsetY * window.GetDpiScale());
			menuStyle.Border = std::max(1.0f, std::round(k_highlightBorder * window.GetDpiScale()));
			menuStyle.Rounding = EditorUI::k_cornerRounding * window.GetDpiScale();
			menuStyle.DrawList->ChannelsSplit(2);
			menuStyle.DrawList->ChannelsSetCurrent(k_contentChannel);

			m_titleBar.Begin(window);

			const float menusLeft = ImGui::GetCursorScreenPos().x;
			if (BeginBarMenu(menuStyle, "File"))
			{
				DrawFileMenu(context);
				ImGui::EndMenu();
			}

			if (BeginBarMenu(menuStyle, "Edit"))
			{
				DrawEditMenu(context);
				ImGui::EndMenu();
			}

			if (BeginBarMenu(menuStyle, "View"))
			{
				context.GetPanels().DrawMenuItems("View");
				ImGui::EndMenu();
			}

			if (BeginBarMenu(menuStyle, "Help"))
			{
				context.GetPanels().DrawMenuItems("Help");
				ImGui::EndMenu();
			}
			m_titleBar.AddControl(menusLeft, ImGui::GetCursorScreenPos().x);

			m_titleBar.End(window);
			menuStyle.DrawList->ChannelsMerge();

			ImGui::EndMainMenuBar();
		}
		ImGui::PopStyleVar();
	}

	void MainMenuBar::DrawFileMenu(EditorContext& context)
	{
		SceneDocument& scene = context.GetSceneDocument();
		GameBuilder& builder = context.GetGameBuilder();

		// Nothing that touches the scene while one is streaming in
		const bool canUseScene = !scene.IsLoading();

		UnsavedChangesPrompt& unsavedChanges = context.GetUnsavedChangesPrompt();

		if (ImGui::MenuItem("New Scene...", nullptr, false, canUseScene))
		{
			unsavedChanges.Request(context, "starting a new scene", [&scene]() { scene.New(); });
		}

		if (ImGui::MenuItem("Load Scene...", nullptr, false, canUseScene))
		{
			// Asked first, so the browser only opens once it's decided to go ahead
			unsavedChanges.Request(context, "loading another scene", [&context, &scene]()
				{
					FileBrowserRequest request;
					request.Title = "Load Scene";
					request.RootFolder = AssetPaths::GetSceneFolder();
					request.Extensions = { AssetPaths::k_sceneExtension };
					request.InitialSelection = scene.HasFilePath() ? scene.GetFilePath() : Filepath();
					request.ThumbnailFor = [](const Filepath& file) { return Thumbnails::Scenes().GetPathFor(file); };
					request.OnChosen = [&scene](const Filepath& file) { scene.Load(file); };
					context.GetThumbnails().AddToBrowser(request);
					context.GetFileBrowser().Open(std::move(request));
				});
		}

		if (EditorUI::MenuItem("Save Scene", "Ctrl+S", canUseScene))
		{
			scene.SaveWithPrompt();
		}

		ImGui::Separator();

		const bool canBuild = !builder.IsBuilding();
		if (ImGui::MenuItem("Build Game...", nullptr, false, canBuild))
		{
			builder.Build();
		}
		// The game loads the scene's file, so unsaved changes are offered a save first
		if (ImGui::MenuItem("Launch Game...", nullptr, false, canBuild && canUseScene))
		{
			unsavedChanges.Request(context, "launching the game", [&scene, &builder]()
				{
					// Read once the prompt is done, since saving a new scene gives it its file. A new scene that isn't
					// saved is still the default one on disk.
					builder.BuildAndLaunch(scene.HasFilePath() ? scene.GetFilePath() : GameScene::GetDefaultScenePath());
				});
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Exit"))
		{
			unsavedChanges.Request(context, "exiting", []() { RZE().PostExit(); });
		}
	}

	void MainMenuBar::DrawEditMenu(EditorContext& context)
	{
		// Nothing that touches the scene while one is streaming in
		const bool canUseScene = !context.GetSceneDocument().IsLoading();
		const bool hasSelection = !context.GetSelection().IsEmpty();

		const UndoHistory& history = context.GetUndoHistory();
		const std::string undoLabel = history.CanUndo() ? "Undo " + history.GetUndoName() : std::string("Undo");
		const std::string redoLabel = history.CanRedo() ? "Redo " + history.GetRedoName() : std::string("Redo");
		if (EditorUI::MenuItem(undoLabel.c_str(), "Ctrl+Z", canUseScene && history.CanUndo()))
		{
			context.GetCommands().Push<UndoCommand>();
		}
		if (EditorUI::MenuItem(redoLabel.c_str(), "Ctrl+Y", canUseScene && history.CanRedo()))
		{
			context.GetCommands().Push<RedoCommand>();
		}

		ImGui::Separator();

		if (EditorUI::MenuItem("Copy", "Ctrl+C", canUseScene && hasSelection))
		{
			Copy(context);
		}
		if (EditorUI::MenuItem("Paste", "Ctrl+V", canUseScene && context.GetClipboard().HasContents()))
		{
			Paste(context);
		}
		if (EditorUI::MenuItem("Duplicate", "Ctrl+D", canUseScene && hasSelection))
		{
			Duplicate(context);
		}

		ImGui::Separator();

		context.GetPanels().DrawMenuItems("Edit");
	}

	void MainMenuBar::HandleShortcuts(EditorContext& context)
	{
		if (context.GetSceneDocument().IsLoading())
		{
			return;
		}

		// Global routes lose to a focused widget that registers the same keys, so text fields keep their own copy/paste
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_C, ImGuiInputFlags_RouteGlobal))
		{
			Copy(context);
		}
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_V, ImGuiInputFlags_RouteGlobal))
		{
			Paste(context);
		}
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_D, ImGuiInputFlags_RouteGlobal))
		{
			Duplicate(context);
		}

		if (!IsEditInProgress())
		{
			if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Z, ImGuiInputFlags_RouteGlobal))
			{
				context.GetCommands().Push<UndoCommand>();
			}
			if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Y, ImGuiInputFlags_RouteGlobal)
				|| ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z, ImGuiInputFlags_RouteGlobal))
			{
				context.GetCommands().Push<RedoCommand>();
			}
			if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, ImGuiInputFlags_RouteGlobal))
			{
				context.GetSceneDocument().SaveWithPrompt();
			}
		}
	}

	void MainMenuBar::Copy(EditorContext& context)
	{
		context.GetClipboard().Copy(context.GetSelection().GetAll());
	}

	void MainMenuBar::Paste(EditorContext& context)
	{
		GameObjectClipboard& clipboard = context.GetClipboard();
		if (clipboard.HasContents())
		{
			const int steps = clipboard.AdvancePasteSteps();
			context.GetCommands().Push<PasteGameObjectsCommand>(clipboard.GetContents(), GetPasteOffset(context, clipboard.GetContentsWidth(), steps));
		}
	}

	void MainMenuBar::Duplicate(EditorContext& context)
	{
		// Copies straight into the paste, leaving the clipboard alone. The copies are selected afterwards, so
		// duplicating again steps on from them.
		const std::vector<GameObjectPtr>& selected = context.GetSelection().GetAll();
		std::vector<GameObjectSnapshot> snapshots = GameObjectClipboard::TakeSnapshots(selected);
		if (!snapshots.empty())
		{
			const float width = GameObjectClipboard::MeasureWidth(selected);
			context.GetCommands().Push<PasteGameObjectsCommand>(std::move(snapshots), GetPasteOffset(context, width, 1));
		}
	}

	Vector3D MainMenuBar::GetPasteOffset(const EditorContext& context, float width, int steps)
	{
		// Along world +X, clear of the originals (and of earlier pastes) with a snap size between them. Objects without
		// a mesh have no width, so they just step by the snap size.
		const float step = width + context.GetSettings().GetSnapSize();
		return Vector3D(step * static_cast<float>(steps), 0.0f, 0.0f);
	}

}
