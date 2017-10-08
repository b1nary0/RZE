#include <StdAfx.h>

#include <Editor/EditorEnvironment.h>
#include <Editor/EditorWorld.h>
#include <Editor/Components/Windows/EditorLogWindow.h>

// #TODO(Josh) Temp
#include <Game/GameEntity.h>

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

void EditorEnvironment::InternalInit()
{
	GameEntity* logEntity = mWorld->AddEntity<GameEntity>();
	EditorLogWindow* logWindow = logEntity->AddComponent<EditorLogWindow>("Log Window");

	const Vector2D& screenSize = RZE_Engine::Get()->GetWindowSettings().GetDimensions();

	logWindow->SetDimensions(Vector2D(400, 200));
	logWindow->SetPosition(Vector2D(10.0f, screenSize.Y() - (logWindow->GetDimensions().Y() + 10)));
}
