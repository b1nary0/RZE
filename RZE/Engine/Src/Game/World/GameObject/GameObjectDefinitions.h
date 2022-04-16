#pragma once

#include <Utils/PrimitiveDefs.h>

typedef U32 GameObjectID;
typedef U32 GameObjectComponentID;

class GameObject;

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

private:
	GameObjectPtr(GameObject* gameObject)
		: m_ptr(gameObject) {}

private:
	GameObject* m_ptr = nullptr;

public:
	static const GameObjectPtr EMPTY;
};
