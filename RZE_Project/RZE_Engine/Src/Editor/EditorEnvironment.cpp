#include <StdAfx.h>

#include <Editor/EditorEnvironment.h>

#include <Editor/EditorWorld.h>

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
