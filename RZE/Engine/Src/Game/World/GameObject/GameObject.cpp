#include <StdAfx.h>
#include <Game/World/GameObject/GameObject.h>

#include <Game/World/GameObject/GameObjectComponent.h>

#include <Game/World/GameObjectComponents/TransformComponent.h>

static GameObjectID s_nextObjectID = 0;

GameObject::GameObject()
	: m_id(s_nextObjectID++)
{
	m_stateFlags.IsInScene = false;
	m_stateFlags.IncludeInSave = true;
}

GameObject::GameObject(const std::string& name)
	: m_id(s_nextObjectID++)
	, m_name(name)
{
}

GameObject::~GameObject()
{
}

void GameObject::Initialize()
{
}

void GameObject::Uninitialize()
{
	for (auto& component : m_components)
	{
		// @TODO component->Uninitialize here?
		delete component;
	}
	m_components.clear();
}

GameObjectComponentBase* GameObject::AddComponentByID(GameObjectComponentID id)
{
	// #TODO Slow function

	auto it = std::find_if(m_components.begin(), m_components.end(),
		[id](const GameObjectComponentBase* component)
		{
			return id == component->m_id;
		});

	if (it == m_components.end())
	{
		GameObjectComponentBase* component = GameObjectComponentRegistry::CreateComponentByID(id);
		component->SetOwner(this);
		component->Initialize();
		m_components.push_back(component);
		return component;
	}

	LOG_CONSOLE("Failed to add component as a component of that type already exists");
	return nullptr;
}

TransformComponent& GameObject::GetTransformComponent()
{
	TransformComponent* transformComponent = GetComponent<TransformComponent>();
	AssertMsg(transformComponent != nullptr, "Illegal for GameObject to not have TransformComponent at any time.");

	return *transformComponent;
}

void GameObject::OnAddToScene()
{
	for (auto& component : m_components)
	{
		component->OnAddToScene();
	}

	m_stateFlags.IsInScene = true;
}

void GameObject::OnRemoveFromScene()
{
	for (auto& component : m_components)
	{
		component->OnRemoveFromScene();
	}

	m_stateFlags.IsInScene = false;
}

void GameObject::SetIncludeInSave(bool include)
{
	m_stateFlags.IncludeInSave = include;
}

void GameObject::Update()
{
	for (auto& component : m_components)
	{
		component->Update();
	}
}

void GameObject::Save(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const
{
	writer.String(GetName().c_str());
	writer.StartObject();
	{
		writer.String("components");
		writer.StartObject();
		{
			for (auto& component : m_components)
			{
				component->Save(writer);
			}
		}
		writer.EndObject();
	}
	writer.EndObject();
}

void GameObject::Load(rapidjson::Value& data)
{
	const GameObjectComponentRegistry::ComponentNameIDMap& componentInfo = GameObjectComponentRegistry::GetAllComponentReflectData();

	// ComponentBegin
	for (auto member = data.MemberBegin(); member != data.MemberEnd(); ++member)
	{
		rapidjson::Value& compVal = member->value;
		for (auto& dataPair : componentInfo)
		{
			rapidjson::Value::MemberIterator compData = compVal.FindMember(dataPair.second.c_str());
			if (compData != compVal.MemberEnd())
			{
				GameObjectComponentBase* component = AddComponentByID(dataPair.first);
				component->Load(compData->value);
			}
		}
	}
}
