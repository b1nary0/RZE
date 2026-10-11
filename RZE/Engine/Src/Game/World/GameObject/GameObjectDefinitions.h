#pragma once

#include <Utils/PrimitiveDefs.h>

#include <cstddef>

typedef U32 GameObjectID;
typedef U32 GameObjectComponentID;
// Unique per component instance, unlike GameObjectComponentID, which identifies the component's type
typedef U32 GameObjectComponentInstanceID;

class GameObject;
class GameObjectComponentBase;

// Tracks which GameObjects and components are alive, so their handles can tell when they've been destroyed.
// IDs are never reused, so a handle to a destroyed object stays invalid even if a new one lands at its address.
// Main thread only, like the rest of the scene.
namespace GameObjectLifetime
{
	constexpr U32 k_invalidID = 0xFFFFFFFF;

	bool IsGameObjectAlive(GameObjectID id);
	bool IsComponentAlive(GameObjectComponentInstanceID id);

	// Debug builds assert that a handle is alive before it's dereferenced
	void CheckGameObjectAccess(GameObjectID id);
	void CheckComponentAccess(GameObjectComponentInstanceID id);
}

// A non-owning handle to a GameObject. Once the object is destroyed the handle is invalid: it compares equal to
// nullptr and dereferencing it asserts in debug builds. Two handles are equal when they refer to the same object.
class GameObjectPtr
{
	friend class GameScene;
	friend class GameObject;

public:
	GameObjectPtr() = default;
	GameObjectPtr(std::nullptr_t) {}
	GameObjectPtr(const GameObjectPtr&) = default;
	~GameObjectPtr() = default;

	GameObjectPtr(GameObjectPtr&& other) noexcept
		: m_ptr(other.m_ptr)
		, m_id(other.m_id)
	{
		other.m_ptr = nullptr;
		other.m_id = GameObjectLifetime::k_invalidID;
	}

	GameObjectPtr& operator=(const GameObjectPtr& other) = default;

	// False once the object has been destroyed
	bool IsValid() const
	{
		return m_ptr != nullptr && GameObjectLifetime::IsGameObjectAlive(m_id);
	}

	GameObject* operator->() const
	{
		CheckAccess();
		return m_ptr;
	}

	GameObject* operator*() const
	{
		CheckAccess();
		return m_ptr;
	}

	bool operator==(const GameObjectPtr& other) const
	{
		return m_id == other.m_id;
	}

	bool operator!=(const GameObjectPtr& other) const
	{
		return !(*this == other);
	}

	bool operator==(std::nullptr_t) const
	{
		return !IsValid();
	}

	bool operator!=(std::nullptr_t) const
	{
		return IsValid();
	}

private:
	// Defined in GameObject.cpp, where GameObject is complete
	GameObjectPtr(GameObject* gameObject);

	void CheckAccess() const
	{
#ifdef _DEBUG
		if (m_ptr != nullptr)
		{
			GameObjectLifetime::CheckGameObjectAccess(m_id);
		}
#endif
	}

private:
	GameObject* m_ptr = nullptr;
	GameObjectID m_id = GameObjectLifetime::k_invalidID;
};

// A non-owning handle to a component, with the same rules as GameObjectPtr: invalid (and equal to nullptr) once the
// component is destroyed, whether it was removed or its GameObject was.
template <typename TComponentType>
class GameObjectComponentPtr
{
	friend class GameObject;
	friend class GameScene;

public:
	GameObjectComponentPtr() = default;
	GameObjectComponentPtr(std::nullptr_t) {}
	GameObjectComponentPtr(const GameObjectComponentPtr&) = default;
	~GameObjectComponentPtr() = default;

	GameObjectComponentPtr(GameObjectComponentPtr<TComponentType>&& other) noexcept
		: m_ptr(other.m_ptr)
		, m_instanceID(other.m_instanceID)
	{
		other.m_ptr = nullptr;
		other.m_instanceID = GameObjectLifetime::k_invalidID;
	}

	GameObjectComponentPtr& operator=(const GameObjectComponentPtr& other) = default;

	// False once the component has been destroyed
	bool IsValid() const
	{
		return m_ptr != nullptr && GameObjectLifetime::IsComponentAlive(m_instanceID);
	}

	TComponentType* operator->() const
	{
		CheckAccess();
		return m_ptr;
	}

	TComponentType* operator*() const
	{
		CheckAccess();
		return m_ptr;
	}

	bool operator==(const GameObjectComponentPtr& other) const
	{
		return m_instanceID == other.m_instanceID;
	}

	bool operator!=(const GameObjectComponentPtr& other) const
	{
		return !(*this == other);
	}

	bool operator==(std::nullptr_t) const
	{
		return !IsValid();
	}

	bool operator!=(std::nullptr_t) const
	{
		return IsValid();
	}

private:
	GameObjectComponentPtr(TComponentType* component)
		: m_ptr(component)
		, m_instanceID(component != nullptr ? component->GetInstanceID() : GameObjectLifetime::k_invalidID) {}

	void CheckAccess() const
	{
#ifdef _DEBUG
		if (m_ptr != nullptr)
		{
			GameObjectLifetime::CheckComponentAccess(m_instanceID);
		}
#endif
	}

private:
	TComponentType* m_ptr = nullptr;
	GameObjectComponentInstanceID m_instanceID = GameObjectLifetime::k_invalidID;

public:
	static const GameObjectComponentPtr EMPTY;
};
