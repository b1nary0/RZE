#pragma once

#include <Utils/LiveObjectPtr.h>
#include <Utils/PrimitiveDefs.h>

// Handed out by LiveIDAllocator, so unique within a session but not in creation order
typedef U64 GameObjectID;
typedef U32 GameObjectComponentID;
// Unique per component instance, unlike GameObjectComponentID, which identifies the component's type
typedef U64 GameObjectComponentInstanceID;

class GameObject;
class GameObjectComponentBase;

// Which GameObjects are alive, for their handles (LiveObjectPtr). IDs are never handed out twice, so a handle to a
// destroyed object stays invalid even if a new one lands at its address. Main thread only, like the rest of the scene.
struct GameObjectLifetime
{
	static bool IsAlive(GameObjectID id);
	static GameObjectID GetID(const GameObject* gameObject);
};

// Which components are alive, for their handles, by instance ID. A component dies when it's removed or its GameObject
// is destroyed.
struct GameObjectComponentLifetime
{
	static bool IsAlive(GameObjectComponentInstanceID id);
	static GameObjectComponentInstanceID GetID(const GameObjectComponentBase* component);
};

// A non-owning handle to a GameObject: invalid (and equal to nullptr) once the object is destroyed
using GameObjectPtr = LiveObjectPtr<GameObject, GameObjectLifetime>;

// A non-owning handle to a component, with the same rules as GameObjectPtr
template <typename TComponentType>
using GameObjectComponentPtr = LiveObjectPtr<TComponentType, GameObjectComponentLifetime>;
