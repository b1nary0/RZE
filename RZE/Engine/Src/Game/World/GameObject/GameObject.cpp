#include <StdAfx.h>
#include <Game/World/GameObject/GameObject.h>

#include <Game/World/GameObject/GameObjectComponent.h>

static GameObjectID s_nextObjectID = 0;

GameObject::GameObject()
	: m_id(s_nextObjectID++)
{
}

GameObject::~GameObject()
{
}

void GameObject::Update()
{
	for (auto& component : m_components)
	{
		component->Update();
	}
}
