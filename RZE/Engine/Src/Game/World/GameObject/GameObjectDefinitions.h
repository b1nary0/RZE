#pragma once

#include <Utils/PrimitiveDefs.h>

typedef U32 GameObjectID;
typedef U32 GameObjectComponentID;

class GameObject;
class GameObjectComponentBase;

class GameObjectPtr
{
	friend class GameScene;

public:
	GameObjectPtr() = default;
	GameObjectPtr(const GameObjectPtr&) = default;
	~GameObjectPtr() = default;
	
	GameObjectPtr(GameObjectPtr&& other)
	{
		m_ptr = other.m_ptr;
		other.m_ptr = nullptr;
	}
	
	GameObject* operator->()
	{
		return m_ptr;
	}

	GameObjectPtr& operator=(const GameObjectPtr& other) = default;

	bool operator==(const GameObjectPtr& other)
	{
		return m_ptr == other.m_ptr;
	}

	bool operator==(const GameObjectPtr* other)
	{
		return this == other;
	}

	bool operator!=(const GameObjectPtr& other)
	{
		return !(*this == other);
	}

	bool operator!=(const std::nullptr_t null)
	{
		return this->m_ptr != null;
	}

	GameObject* operator*() const
	{
		return m_ptr;
	}

private:
	GameObjectPtr(GameObject* gameObject)
		: m_ptr(gameObject) {}

private:
	GameObject* m_ptr = nullptr;

public:
	static const GameObjectPtr EMPTY;
};

template <typename TComponentType>
class GameObjectComponentPtr
{
	friend class GameObject;
	friend class GameScene;

public:
	GameObjectComponentPtr() = default;
	GameObjectComponentPtr(const GameObjectComponentPtr&) = default;
	~GameObjectComponentPtr() = default;

	GameObjectComponentPtr(GameObjectComponentPtr<TComponentType>&& other) noexcept
	{
		m_ptr = other.m_ptr;
		other.m_ptr = nullptr;
	}

	TComponentType* operator->()
	{
		return m_ptr;
	}

	GameObjectComponentPtr& operator=(const GameObjectComponentPtr& other) = default;

	bool operator==(const GameObjectComponentPtr& other)
	{
		return m_ptr == other.m_ptr;
	}

	bool operator==(const GameObjectComponentPtr* other)
	{
		return this == other;
	}

	bool operator!=(const GameObjectComponentPtr& other) const
	{
		return !(*this == other);
	}

	bool operator!=(const std::nullptr_t null)
	{
		return this->m_ptr != null;
	}

	TComponentType* operator*() const
	{
		return m_ptr;
	}

private:
	GameObjectComponentPtr(TComponentType* component)
		: m_ptr(component) {}

private:
	TComponentType* m_ptr = nullptr;

public:
	static const GameObjectComponentPtr EMPTY;
};