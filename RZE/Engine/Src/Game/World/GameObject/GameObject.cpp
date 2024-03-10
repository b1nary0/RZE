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

	RZE_LOG("Failed to add component as a component of that type already exists");
	return nullptr;
}

GameObjectComponentPtr<TransformComponent> GameObject::GetTransformComponent()
{
	// @todo should store GameObjectComponent<TransformComponent> as member variable 
	// to avoid doing the search incurred by GetComponent<>
	GameObjectComponentPtr<TransformComponent> transformComponent = GetComponent<TransformComponent>();
	AssertMsg(transformComponent != nullptr, "Illegal for GameObject to not have TransformComponent at any time.");

	return transformComponent;
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
	// @todo maybe do IsDirty() or something
	for (auto& component : m_components)
	{
		component->Update();
	}

	for (auto& child : m_children)
	{
		child->Update();
	}
}

bool GameObject::IsRoot()
{
	return GetTransformComponent()->IsRoot();
}

void GameObject::AttachTo(GameObject* gameObject)
{
	AssertNotNull(gameObject);
	gameObject->AddChild(this);
	m_parent = gameObject;
}

void GameObject::DetachFromParent()
{
	AssertNotNull(m_parent);
	m_parent->RemoveChild(this);
	m_parent = nullptr;
}

void GameObject::AddChild(GameObject* child)
{
#ifdef _DEBUG
	auto existingChild = std::find_if(m_children.begin(), m_children.end(), [&child](GameObject* gameObject)
		{
			return gameObject == child;
		});

	AssertExpr(existingChild == m_children.end());
#endif

	child->GetTransformComponent()->AttachTo(*GetTransformComponent());
	m_children.push_back(child);
	// @todo OnChildAdded/OnAttachToParent signal
}

void GameObject::RemoveChild(GameObject* child)
{
	auto existingChild = std::find_if(m_children.begin(), m_children.end(), [&child](GameObject* gameObject)
		{
			return gameObject == child;
		});

	if (existingChild != m_children.end())
	{
		// eraseback
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
				component->Serialize(writer);
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
				component->Deserialize(compData->value);
			}
		}
	}
}
