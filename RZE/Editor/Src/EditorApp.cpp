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
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5.0f, 5.0f));
	if (ImGui::BeginMainMenuBar())
	{
		ImGui::MenuItem("Test");
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
