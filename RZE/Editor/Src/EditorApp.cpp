#include <EditorApp.h>

#include <ECS/Systems/FreeCameraSystem.h>

#include <ImGui/imgui.h>
#include <Optick/optick.h>

namespace Editor
{
	EditorApp::EditorApp()
	{
	}

	EditorApp::~EditorApp()
	{
	}

	static ImFont* ubuntu = nullptr;
	void EditorApp::Initialize()
	{
		GetWindow()->SetWindowSize(Vector2D(1920.0f, 1080.0f));

		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		ImGui::GetIO().KeyRepeatDelay = 0.5f;

		LoadFonts();
		StyleSetup();
	}

	void EditorApp::Start()
	{
		RZE().GetActiveScene().Load(FilePath("Assets/Scenes/TestGame.scene"));
	}

	void EditorApp::Update()
	{
		OPTICK_EVENT();

		ImGui::PushFont(mFontMapping.at("ubuntu_medium"));

		DisplayMenuBar();
		HandleGeneralContextMenu();

		ResolvePanelState();
		ImGui::PopFont();
	}

	void EditorApp::ShutDown()
	{
	}

	void EditorApp::RegisterInputEvents(InputHandler& inputHandler)
	{
	}

	void EditorApp::DisplayMenuBar()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.5f, 5.0f));
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Load Scene..."))
				{
					FilePath newScenePath = RZE_Application::RZE().ShowOpenFilePrompt();
					if (newScenePath.IsValid())
					{
						RZE_Application::RZE().GetActiveScene().Load(newScenePath);
					}
				}

				if (ImGui::MenuItem("Save Scene"))
				{

				}

				if (ImGui::MenuItem("Exit"))
				{
					RZE().PostExit();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View"))
			{
				if (ImGui::BeginMenu("Windows"))
				{
					if (ImGui::MenuItem("Scene"))
					{
						mScenePanel.Enable();
					}
					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Help"))
			{
				if (ImGui::MenuItem("Show ImGui Demo Window"))
				{
					mPanelStates.bDemoPanelEnabled = true;
				}

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}
		ImGui::PopStyleVar();
	}

	void EditorApp::HandleGeneralContextMenu()
	{
		if (ImGui::BeginPopupContextVoid())
		{
			if (ImGui::BeginMenu("View"))
			{
				if (ImGui::BeginMenu("Windows"))
				{
					if (ImGui::MenuItem("Scene"))
					{
						mScenePanel.Enable();
					}
					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Create Entity"))
			{
				// #TODO(Do this better)
				static int sGenericEntityCount = 0;

				std::string newEntityStr = StringUtils::FormatString("Entity%i", sGenericEntityCount);
				RZE_Application::RZE().GetActiveScene().CreateEntity(newEntityStr);

				++sGenericEntityCount;
			}

			ImGui::EndPopup();
		}
	}

	void EditorApp::ResolvePanelState()
	{
		if (mPanelStates.bDemoPanelEnabled)
		{
			ImGui::ShowDemoWindow(&mPanelStates.bDemoPanelEnabled);
		}

		if (mScenePanel.IsEnabled())
		{
			mScenePanel.Display();
		}
	}

	void EditorApp::LoadFonts()
	{
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->AddFontDefault();

		FilePath ubuntuPath("Assets/Fonts/Ubuntu-Medium.ttf");
		FilePath arialPath("Assets/Fonts/Arial.ttf");
		FilePath consolasPath("Assets/Fonts/Consolas.ttf");

		mFontMapping.insert({"ubuntu_medium", io.Fonts->AddFontFromFileTTF(ubuntuPath.GetAbsolutePath().c_str(), 14)});
		mFontMapping.insert({"arial", io.Fonts->AddFontFromFileTTF(arialPath.GetAbsolutePath().c_str(), 15)});
		mFontMapping.insert({"consolas", io.Fonts->AddFontFromFileTTF(consolasPath.GetAbsolutePath().c_str(), 15)});

		io.Fonts->Build();
	}

	void EditorApp::StyleSetup()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		style.FrameRounding = 0.0f;
		style.WindowRounding = 0.0f;

		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
		
		style.Colors[ImGuiCol_Header] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
	}

}
