#include <StdAfx.h>

#include <Editor/EditorWorld.h>
#include <Editor/Systems/EditorLogSystem.h>
#include <Editor/Systems/EditorUISystem.h>

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
	InternalAddSystem<EditorUISystem>(this);
	InternalAddSystem<EditorLogSystem>(this);

	for (auto& system : InternalGetSystems())
	{
		system->Init();
	}
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
