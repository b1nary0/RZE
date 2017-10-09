#include <StdAfx.h>

#include <Editor/EditorEnvironment.h>
#include <Editor/EditorWorld.h>
#include <Editor/EditorEntity.h>

#include <Editor/Components/Windows/EditorLogWindow.h>

EditorEnvironment::EditorEnvironment()
{
}

EditorEnvironment::~EditorEnvironment()
{

}

void EditorEnvironment::Init()
{
	mWorld = new EditorWorld();
	AssertNotNull(mWorld);
	mWorld->Init();

	InternalInit();
}

void EditorEnvironment::Update()
{
	mWorld->Update();
}

void EditorEnvironment::Shutdown()
{
	mWorld->ShutDown();
	delete mWorld;
}

void EditorEnvironment::PostToLogWindow(const std::string& msg)
{
	mLogWindowEnt->GetComponent<EditorLogWindow>()->AddItem(msg);
}

void EditorEnvironment::InternalInit()
{
	mLogWindowEnt = mWorld->AddEntity<EditorEntity>();
	EditorLogWindow* logWindow = mLogWindowEnt->AddComponent<EditorLogWindow>("Log Window");

	const Vector2D& screenSize = RZE_Engine::Get()->GetWindowSettings().GetDimensions();

	logWindow->SetDimensions(Vector2D(600, 250));
	logWindow->SetPosition(Vector2D(10.0f, screenSize.Y() - (logWindow->GetDimensions().Y() + 10)));
}
