#include <StdAfx.h>

#include <Editor/EditorWorld.h>

#include <Game/ECS/EntitySystem.h>

EditorWorld::EditorWorld()
{
}

EditorWorld::~EditorWorld()
{
}

RZE_Renderer* const EditorWorld::GetRenderer() const
{
	return nullptr;
}

void EditorWorld::Init()
{

}

void EditorWorld::Update()
{
	for (auto& system : InternalGetSystems())
	{
		system->Update();
	}
}

void EditorWorld::ShutDown()
{

}
