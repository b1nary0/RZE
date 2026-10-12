#include <StdAfx.h>
#include <Game/World/GameObject/GameObject.h>

#include <Game/World/GameObject/GameObjectComponent.h>
#include <Game/World/GameObject/LiveIDSet.h>

#include <Game/World/GameObjectComponents/RenderComponent.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Utils/Math/Math.h>
#include <Utils/Utils.h>

#include <cfloat>

namespace
{
	// Never reset, so IDs are never reused (see GameObjectLifetime)
	GameObjectID s_nextObjectID = 0;

	LiveIDSet s_aliveGameObjects;
}

namespace GameObjectLifetime
{
	bool IsGameObjectAlive(GameObjectID id)
	{
		return s_aliveGameObjects.Contains(id);
	}

	void CheckGameObjectAccess(GameObjectID id)
	{
		AssertMsg(IsGameObjectAlive(id), "GameObjectPtr dereferenced after its GameObject was destroyed");
	}
}

GameObjectPtr::GameObjectPtr(GameObject* gameObject)
	: m_ptr(gameObject)
	, m_id(gameObject != nullptr ? gameObject->GetID() : GameObjectLifetime::k_invalidID)
{
}

GameObject::GameObject(ObjectGUID guid)
	: m_id(s_nextObjectID++)
	, m_guid(guid)
{
	m_stateFlags.IsInScene = false;
	m_stateFlags.IncludeInSave = true;

	s_aliveGameObjects.Add(m_id);
}

GameObject::GameObject(const std::string& name, ObjectGUID guid)
	: m_id(s_nextObjectID++)
	, m_guid(guid)
	, m_name(name)
{
	m_stateFlags.IsInScene = false;
	m_stateFlags.IncludeInSave = true;

	s_aliveGameObjects.Add(m_id);
}

GameObject::~GameObject()
{
	s_aliveGameObjects.Remove(m_id);
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
		if (m_stateFlags.IsInScene)
		{
			component->OnAddToScene();
		}
		return component;
	}

	RZE_LOG("Failed to add component as a component of that type already exists");
	return nullptr;
}

void GameObject::GetComponentHandles(std::vector<GameObjectComponentPtr<GameObjectComponentBase>>& outHandles) const
{
	outHandles.clear();
	outHandles.reserve(m_components.size());
	for (GameObjectComponentBase* component : m_components)
	{
		outHandles.push_back(GameObjectComponentPtr<GameObjectComponentBase>(component));
	}
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

bool GameObject::IsSelfOrAncestorOf(const GameObjectPtr& gameObject) const
{
	// Up from gameObject, which is a single chain, rather than down through this object's whole subtree
	for (const GameObject* current = *gameObject; current != nullptr; current = current->m_parent != nullptr ? *current->m_parent : nullptr)
	{
		if (current == this)
		{
			return true;
		}
	}

	return false;
}

int GameObject::NumChildren()
{
	return m_children.size();
}

bool GameObject::HasChildren()
{
	return m_children.size();
}

GameObjectPtr GameObject::GetChildAtIndex(int index)
{
	AssertExpr(index < m_children.size());
	return m_children[index];
}

bool GameObject::GetHierarchyBounds(Vector3D& outMin, Vector3D& outMax)
{
	outMin = Vector3D(FLT_MAX);
	outMax = Vector3D(-FLT_MAX);
	return GrowHierarchyBounds(outMin, outMax);
}

bool GameObject::GrowHierarchyBounds(Vector3D& inOutMin, Vector3D& inOutMax)
{
	bool foundAny = false;

	GameObjectComponentPtr<RenderComponent> renderComponent = GetComponent<RenderComponent>();
	Vector3D meshMin;
	Vector3D meshMax;
	if (renderComponent != nullptr && renderComponent->GetWorldBounds(meshMin, meshMax))
	{
		inOutMin = VectorUtils::Min(inOutMin, meshMin);
		inOutMax = VectorUtils::Max(inOutMax, meshMax);
		foundAny = true;
	}

	for (GameObjectPtr child : m_children)
	{
		foundAny |= child->GrowHierarchyBounds(inOutMin, inOutMax);
	}

	return foundAny;
}

void GameObject::AttachTo(GameObjectPtr gameObject)
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
	auto existingChild = std::find_if(m_children.begin(), m_children.end(), [&child](GameObjectPtr gameObject)
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
	auto existingChild = std::find_if(m_children.begin(), m_children.end(), [&child](GameObjectPtr gameObject)
		{
			return gameObject == child;
		});

	if (existingChild != m_children.end())
	{
		(*existingChild)->GetTransformComponent()->DetachFromParent();
		ContainerUtils::VectorEraseBack(m_children, existingChild);
	}
}

void GameObject::Save(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const
{
	writer.StartObject();
	{
		writer.Key("id");
		writer.String(ObjectGUIDs::ToString(m_guid).c_str());

		writer.Key("name");
		writer.String(GetName().c_str());

		if (m_parent != nullptr)
		{
			writer.Key("parent");
			writer.String(ObjectGUIDs::ToString(m_parent->GetGUID()).c_str());
		}

		writer.Key("components");
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

	// After this object, so a child's parent always comes first
	for (auto& child : m_children)
	{
		child->Save(writer);
	}
}

void GameObject::LoadComponents(const rapidjson::Value& entry)
{
	const auto components = entry.FindMember("components");
	if (components == entry.MemberEnd() || !components->value.IsObject())
	{
		return;
	}

	const GameObjectComponentRegistry::ComponentNameIDMap& componentInfo = GameObjectComponentRegistry::GetAllComponentReflectData();
	for (auto& dataPair : componentInfo)
	{
		const auto componentData = components->value.FindMember(dataPair.second.c_str());
		if (componentData != components->value.MemberEnd())
		{
			GameObjectComponentBase* component = AddComponentByID(dataPair.first);
			component->Deserialize(componentData->value);
		}
	}
}
