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
	ImGui::Text("WOLOLOLO");
}

void EditorApp::ShutDown()
{
}

void EditorApp::RegisterInputEvents(InputHandler& inputHandler)
{
}
