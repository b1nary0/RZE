#include <EditorApp.h>

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
}

void EditorApp::ShutDown()
{
}

void EditorApp::RegisterInputEvents(InputHandler& inputHandler)
{
}
