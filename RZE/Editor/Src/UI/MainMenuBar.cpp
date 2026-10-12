#include <UI/MainMenuBar.h>

#include <UI/Framework/EditorContext.h>

#include <RZE.h>

#include <Rendering/MemArena.h>

#include <Windowing/Win32Window.h> // FilePromptParams

#include <Utils/Memory/MemoryUtils.h>
#include <Utils/Platform/Filepath.h>

#include <ImGui/imgui.h>

namespace
{
	const Filepath k_sceneFileDirectoryPath("Assets/Scenes");

	// How many frames the frame time readout holds each sample for
	constexpr int k_frameTimeSampleInterval = 15;
}

namespace Editor
{
	void MainMenuBar::Draw(EditorContext& context)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.5f, 5.0f));
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				DrawFileMenu(context);
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
