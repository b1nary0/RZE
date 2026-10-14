#pragma once

#include <Utils/PrimitiveDefs.h>

#include <cstddef>

class RenderObject;
class LightObject;

// Tracks which render objects and lights are alive, so their handles can tell when they've been destroyed, whether
// through the handle or not (e.g. RenderScene::ClearObjects, or the scene going away). IDs are never reused, so a
// handle to a destroyed object stays invalid even if a new one lands at its address. Main thread only.
namespace RenderSceneLifetime
{
	constexpr U32 k_invalidID = 0xFFFFFFFF;

	bool IsRenderObjectAlive(U32 id);
	bool IsLightObjectAlive(U32 id);

	// Debug builds assert that a handle is alive before it's dereferenced
	void CheckAccess(bool isAlive);
}

// The handle to an object in a RenderScene, held by whatever made it (e.g. RenderComponent), which destroys it through
// RenderScene::DestroyRenderObject / DestroyLightObject. Move-only, so an object has one holder; code that only looks
// at it borrows the holder's handle by reference.
//
// Once the object is destroyed the handle is invalid: it compares equal to nullptr and dereferencing it asserts in
// debug builds.
template <typename TObject, bool (*IsAlive)(U32)>
class RenderSceneObjectPtr
{
	friend class RenderScene;

public:
	RenderSceneObjectPtr() = default;
	~RenderSceneObjectPtr() = default;

	RenderSceneObjectPtr(const RenderSceneObjectPtr&) = delete;
	RenderSceneObjectPtr& operator=(const RenderSceneObjectPtr&) = delete;

	RenderSceneObjectPtr(RenderSceneObjectPtr&& other) noexcept
		: m_ptr(other.m_ptr)
		, m_id(other.m_id)
	{
		other.Reset();
	}

	RenderSceneObjectPtr& operator=(RenderSceneObjectPtr&& other) noexcept
	{
		if (this != &other)
		{
			m_ptr = other.m_ptr;
			m_id = other.m_id;
			other.Reset();
		}
		return *this;
	}

public:
	// False once the object has been destroyed
	bool IsValid() const
	{
		return m_ptr != nullptr && IsAlive(m_id);
	}

	TObject* operator->()
	{
		CheckAccess();
		return m_ptr;
	}

	// Read-only access through a const handle
	const TObject* operator->() const
	{
		CheckAccess();
		return m_ptr;
	}

	bool operator==(const RenderSceneObjectPtr& other) const
	{
		return m_id == other.m_id;
	}

	bool operator!=(const RenderSceneObjectPtr& other) const
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
	RenderSceneObjectPtr(TObject* object, U32 id)
		: m_ptr(object)
		, m_id(id)
	{
	}

	void Reset()
	{
		m_ptr = nullptr;
		m_id = RenderSceneLifetime::k_invalidID;
	}

	void CheckAccess() const
	{
#ifdef _DEBUG
		if (m_ptr != nullptr)
		{
			RenderSceneLifetime::CheckAccess(IsAlive(m_id));
		}
#endif
	}

private:
	TObject* m_ptr = nullptr;
	U32 m_id = RenderSceneLifetime::k_invalidID;
};

using RenderObjectPtr = RenderSceneObjectPtr<RenderObject, &RenderSceneLifetime::IsRenderObjectAlive>;
using LightObjectPtr = RenderSceneObjectPtr<LightObject, &RenderSceneLifetime::IsLightObjectAlive>;
