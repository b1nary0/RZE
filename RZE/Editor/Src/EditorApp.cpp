#include <EditorApp.h>

#include <ImGui/imgui.h>

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

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontDefault();
	FilePath ubuntuPath("Assets/Fonts/Ubuntu-Medium.ttf");

	ubuntu = io.Fonts->AddFontFromFileTTF(ubuntuPath.GetAbsolutePath().c_str(), 15);
	io.Fonts->Build();
}

void EditorApp::Start()
{
}

void EditorApp::Update()
{
	ImGui::PushFont(ubuntu);

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
					mPanelStates.bScenePanelEnabled = true;
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
					mPanelStates.bScenePanelEnabled = true;
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

	if (mPanelStates.bScenePanelEnabled)
	{
		if (ImGui::Begin("Scene", &mPanelStates.bScenePanelEnabled))
		{

		}
		ImGui::End();
	}
}
