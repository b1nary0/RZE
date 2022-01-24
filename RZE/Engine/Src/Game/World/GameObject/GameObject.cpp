#include <StdAfx.h>
#include <Game/World/GameObject/GameObject.h>

#include <Game/World/GameObject/GameObjectComponent.h>

static GameObjectID s_nextObjectID = 0;

GameObject::GameObject()
	: m_id(s_nextObjectID++)
{
}

GameObject::GameObject(const std::string& name)
	: m_id(s_nextObjectID++)
	, m_name(name)
{
}

GameObject::~GameObject()
{
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
		m_components.push_back(component);
		return component;
	}

	LOG_CONSOLE("Failed to add component as a component of that type already exists");
	return nullptr;
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
	for (auto& member = data.MemberBegin(); member != data.MemberEnd(); ++member)
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
