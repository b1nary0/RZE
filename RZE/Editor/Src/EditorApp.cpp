#include <EditorApp.h>

#include <ImGui/imgui.h>

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
		RZE().GetActiveScene().Load(FilePath("Assets/Scenes/TestGame.scene"));
		GetWindow()->SetWindowSize(Vector2D(1920.0f, 1080.0f));

		LoadFonts();
		StyleSetup();
	}

	void EditorApp::Start()
	{
	}

	void EditorApp::Update()
	{
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

		mFontMapping.insert({"ubuntu_medium", io.Fonts->AddFontFromFileTTF(ubuntuPath.GetAbsolutePath().c_str(), 15)});
		mFontMapping.insert({"arial", io.Fonts->AddFontFromFileTTF(arialPath.GetAbsolutePath().c_str(), 15)});
		mFontMapping.insert({"consolas", io.Fonts->AddFontFromFileTTF(consolasPath.GetAbsolutePath().c_str(), 15)});

		io.Fonts->Build();
	}

	void EditorApp::StyleSetup()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		style.FrameRounding = 0.0f;
	}

}
