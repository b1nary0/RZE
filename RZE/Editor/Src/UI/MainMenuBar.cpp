#include <UI/MainMenuBar.h>

#include <UI/Framework/EditorContext.h>

#include <Commands/HistoryCommands.h>
#include <Commands/SceneCommands.h>

#include <RZE.h>

#include <Rendering/MemArena.h>

#include <Windowing/Win32Window.h> // FilePromptParams

#include <Utils/Memory/MemoryUtils.h>
#include <Utils/Platform/Filepath.h>

#include <ImGui/imgui.h>
#include <imGUI/ImGuizmo.h>

namespace
{
	const Filepath k_sceneFileDirectoryPath("Assets/Scenes");

	// Undo and redo wait while a widget or the gizmo is held, so a drag isn't half undone and then carried on. A text
	// field being edited also handles Ctrl+Z itself.
	bool IsEditInProgress()
	{
		return ImGui::IsAnyItemActive() || ImGuizmo::IsUsing();
	}

	// How many frames the frame time readout holds each sample for
	constexpr int k_frameTimeSampleInterval = 15;
}

namespace Editor
{
	void MainMenuBar::Draw(EditorContext& context)
	{
		HandleEditShortcuts(context);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.5f, 5.0f));
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				DrawFileMenu(context);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Edit"))
			{
				DrawEditMenu(context);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View"))
			{
				context.GetPanels().DrawMenuItems("View");
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Help"))
			{
				context.GetPanels().DrawMenuItems("Help");
				ImGui::EndMenu();
			}

			DrawStatus();

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

		if (ImGui::MenuItem("New Scene...", nullptr, false, canUseScene))
		{
			scene.New();
		}

		if (ImGui::MenuItem("Load Scene...", nullptr, false, canUseScene))
		{
			const FilePromptParams openFileParams =
			{
				"RZE Scene",
				k_sceneFileDirectoryPath,
				"*.scene"
			};

			std::string chosenPath;
			if (RZE().ShowOpenFilePrompt(openFileParams, chosenPath))
			{
				scene.Load(Filepath::FromAbsolutePathStr(chosenPath));
			}
		}

		if (ImGui::MenuItem("Save Scene", nullptr, false, canUseScene))
		{
			if (scene.HasFilePath())
			{
				scene.Save();
			}
			else
			{
				const FilePromptParams saveFileParams =
				{
					"Save Scene",
					k_sceneFileDirectoryPath,
					"*.scene"
				};

				std::string chosenPath;
				if (RZE().ShowSaveFilePrompt(saveFileParams, chosenPath))
				{
					scene.SaveAs(Filepath::FromAbsolutePathStr(chosenPath));
				}
			}
		}

		ImGui::Separator();

		const bool canBuild = !builder.IsBuilding();
		if (ImGui::MenuItem("Build Game...", nullptr, false, canBuild))
		{
			builder.Build(false);
		}
		if (ImGui::MenuItem("Launch Game...", nullptr, false, canBuild))
		{
			builder.Build(true);
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Exit"))
		{
			RZE().PostExit();
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
		if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, canUseScene && history.CanUndo()))
		{
			context.GetCommands().Push<UndoCommand>();
		}
		if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y", false, canUseScene && history.CanRedo()))
		{
			context.GetCommands().Push<RedoCommand>();
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Copy", "Ctrl+C", false, canUseScene && hasSelection))
		{
			Copy(context);
		}
		if (ImGui::MenuItem("Paste", "Ctrl+V", false, canUseScene && context.GetClipboard().HasContents()))
		{
			Paste(context);
		}
		if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, canUseScene && hasSelection))
		{
			Duplicate(context);
		}
	}

	void MainMenuBar::HandleEditShortcuts(EditorContext& context)
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
		std::string sceneData = GameObjectClipboard::Serialize(selected);
		if (!sceneData.empty())
		{
			const float width = GameObjectClipboard::MeasureWidth(selected);
			context.GetCommands().Push<PasteGameObjectsCommand>(std::move(sceneData), GetPasteOffset(context, width, 1));
		}
	}

	Vector3D MainMenuBar::GetPasteOffset(const EditorContext& context, float width, int steps)
	{
		// Along world +X, clear of the originals (and of earlier pastes) with a snap size between them. Objects without
		// a mesh have no width, so they just step by the snap size.
		const float step = width + context.GetSettings().GetSnapSize();
		return Vector3D(step * static_cast<float>(steps), 0.0f, 0.0f);
	}

	void MainMenuBar::DrawStatus()
	{
		ImGui::Separator();

		if (m_framesSinceSample > k_frameTimeSampleInterval)
		{
			m_sampledFrameTimeMS = RZE().GetDeltaTimeMS();
			m_framesSinceSample = 0;
		}
		else
		{
			++m_framesSinceSample;
		}

		const float imguiCursorPos = ImGui::GetCursorPosX();
		ImGui::Text("%.1f ms", m_sampledFrameTimeMS);

		ImGui::Separator();

		ImVec4 colour;
		switch (Rendering::MemArena::GetPressureValue())
		{
		case Rendering::MemArena::PressureValue::MED:
			colour = ImVec4(255.0f, 255.0f, 0.0f, 1.0f);
			break;

		case Rendering::MemArena::PressureValue::HIGH:
			colour = ImVec4(255.0f, 0.0f, 0.0f, 1.0f);
			break;

		case Rendering::MemArena::PressureValue::LOW:
		default:
			colour = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
			break;
		}

		const size_t lastFrameAllocationSize = Rendering::MemArena::GetTotalMemoryAllocatedLastFrame();
		const size_t memArenaTotalSizeBytes = Rendering::MemArena::GetSize();

		const float lastFrameSizeKB = static_cast<float>(lastFrameAllocationSize / 1024);
		const float memArenaSizeKB = static_cast<float>(memArenaTotalSizeBytes / 1024);
		const float memArenaSizeMB = static_cast<float>(memArenaTotalSizeBytes / 1024) / 1024;

		const bool isArenaInMB = memArenaTotalSizeBytes >= MemoryUtils::Megabytes(1);
		const float memArenaValue = isArenaInMB ? memArenaSizeMB : memArenaSizeKB;
		const char* const memArenaSizeMagnitudeStr = isArenaInMB ? "MB" : "KB";

		const ImVec2 textSize = ImGui::CalcTextSize("12.0ms ");
		ImGui::SameLine(imguiCursorPos + textSize.x);
		ImGui::Text("RenderCommand MemArena:");
		ImGui::TextColored(colour, "%.2f KB / %.2f %s", lastFrameSizeKB, memArenaValue, memArenaSizeMagnitudeStr);
		ImGui::Text("Peak Used: %d KB", static_cast<int>(Rendering::MemArena::GetPeakUsedBytes() / 1024));
	}
}
