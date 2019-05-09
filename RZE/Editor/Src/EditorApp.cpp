#include <EditorApp.h>

#include <ImGui/imgui.h>

EditorApp::EditorApp()
{
}

EditorApp::~EditorApp()
{
}

void EditorApp::Initialize()
{
	RZE().GetActiveScene().Load(FilePath("Assets/Scenes/TestGame.scene"));
}

void EditorApp::Start()
{
}

void EditorApp::Update()
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
		ImGui::EndMainMenuBar();
	}
	ImGui::PopStyleVar();
}

void EditorApp::ShutDown()
{
}

void EditorApp::RegisterInputEvents(InputHandler& inputHandler)
{
}
