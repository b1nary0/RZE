#pragma once

#include <Utils/DebugUtils/DebugConfig.h>
#include <Utils/LiveIDAllocator.h>
#include <Utils/PrimitiveDefs.h>

#include <cstddef>

namespace LiveObjectPtrDetail
{
	// Asserts that a handle's object is still alive when the handle is used
	void CheckAccess(bool isAlive);
}

// A non-owning handle to an object whose ID comes from a LiveIDAllocator (e.g. GameObjectPtr). Once the object is
// destroyed the handle is invalid: it compares equal to nullptr, and using it asserts (in every build but Retail). Two handles are
// equal when they refer to the same object. Main thread only, like the allocators.
//
// TLifetime says whether an ID is alive and what ID an object has:
//   static bool IsAlive(U64 id);
//   static U64 GetID(const TBase* object);   // TBase is T or a base of it
template <typename T, typename TLifetime>
class LiveObjectPtr
{
public:
	LiveObjectPtr() = default;
	LiveObjectPtr(std::nullptr_t) {}
	// Any live object, or null. Its ID is read from it, so any code holding the object can make a handle to it.
	explicit LiveObjectPtr(T* object);

	LiveObjectPtr(const LiveObjectPtr&) = default;
	LiveObjectPtr& operator=(const LiveObjectPtr&) = default;
	// Moving leaves other null
	LiveObjectPtr(LiveObjectPtr&& other) noexcept;
	LiveObjectPtr& operator=(LiveObjectPtr&& other) noexcept;

public:
	// False once the object has been destroyed
	bool IsValid() const { return m_ptr != nullptr && TLifetime::IsAlive(m_id); }

	T* Get() const;
	T* operator->() const { return Get(); }
	// The object, as a pointer
	T* operator*() const { return Get(); }

	bool operator==(const LiveObjectPtr& other) const { return m_id == other.m_id; }
	bool operator!=(const LiveObjectPtr& other) const { return m_id != other.m_id; }
	bool operator==(std::nullptr_t) const { return !IsValid(); }
	bool operator!=(std::nullptr_t) const { return IsValid(); }
	// Whether the handle's object is alive and is object. A dead handle matches only null, even if a new object has its
	// address, and comparing doesn't assert. Also object == handle.
	bool operator==(const T* object) const;
	bool operator!=(const T* object) const { return !(*this == object); }

private:
	T* m_ptr = nullptr;
	U64 m_id = LiveIDAllocator::k_invalidID;
};

template <typename T, typename TLifetime>
bool operator==(const T* object, const LiveObjectPtr<T, TLifetime>& handle) { return handle == object; }
template <typename T, typename TLifetime>
bool operator!=(const T* object, const LiveObjectPtr<T, TLifetime>& handle) { return handle != object; }

// The one handle to an object, held by whatever made it, which destroys the object through its owner (e.g.
// RenderObjectPtr, which RenderComponent holds and destroys through RenderScene). Move-only, so code that only looks at
// the object borrows the holder's handle by reference; a const handle gives only const access. Otherwise like
// LiveObjectPtr.
template <typename T, typename TLifetime>
class UniqueLiveObjectPtr
{
public:
	UniqueLiveObjectPtr() = default;
	explicit UniqueLiveObjectPtr(T* object) : m_handle(object) {}

	UniqueLiveObjectPtr(const UniqueLiveObjectPtr&) = delete;
	UniqueLiveObjectPtr& operator=(const UniqueLiveObjectPtr&) = delete;
	// Moving leaves other null
	UniqueLiveObjectPtr(UniqueLiveObjectPtr&&) noexcept = default;
	UniqueLiveObjectPtr& operator=(UniqueLiveObjectPtr&&) noexcept = default;

public:
	// False once the object has been destroyed
	bool IsValid() const { return m_handle.IsValid(); }

	T* Get() { return m_handle.Get(); }
	const T* Get() const { return m_handle.Get(); }
	T* operator->() { return m_handle.Get(); }
	const T* operator->() const { return m_handle.Get(); }

	bool operator==(const UniqueLiveObjectPtr& other) const { return m_handle == other.m_handle; }
	bool operator!=(const UniqueLiveObjectPtr& other) const { return m_handle != other.m_handle; }
	bool operator==(std::nullptr_t) const { return !IsValid(); }
	bool operator!=(std::nullptr_t) const { return IsValid(); }
	// As LiveObjectPtr's
	bool operator==(const T* object) const { return m_handle == object; }
	bool operator!=(const T* object) const { return m_handle != object; }

private:
	LiveObjectPtr<T, TLifetime> m_handle;
};

template <typename T, typename TLifetime>
bool operator==(const T* object, const UniqueLiveObjectPtr<T, TLifetime>& handle) { return handle == object; }
template <typename T, typename TLifetime>
bool operator!=(const T* object, const UniqueLiveObjectPtr<T, TLifetime>& handle) { return handle != object; }

template <typename T, typename TLifetime>
LiveObjectPtr<T, TLifetime>::LiveObjectPtr(T* object)
	: m_ptr(object)
	, m_id(object != nullptr ? TLifetime::GetID(object) : LiveIDAllocator::k_invalidID)
{
}

template <typename T, typename TLifetime>
LiveObjectPtr<T, TLifetime>::LiveObjectPtr(LiveObjectPtr&& other) noexcept
	: m_ptr(other.m_ptr)
	, m_id(other.m_id)
{
	other.m_ptr = nullptr;
	other.m_id = LiveIDAllocator::k_invalidID;
}

template <typename T, typename TLifetime>
LiveObjectPtr<T, TLifetime>& LiveObjectPtr<T, TLifetime>::operator=(LiveObjectPtr&& other) noexcept
{
	if (this != &other)
	{
		m_ptr = other.m_ptr;
		m_id = other.m_id;
		other.m_ptr = nullptr;
		other.m_id = LiveIDAllocator::k_invalidID;
	}
	return *this;
}

template <typename T, typename TLifetime>
T* LiveObjectPtr<T, TLifetime>::Get() const
{
#ifdef RZE_ASSERTS_ENABLED
	if (m_ptr != nullptr)
	{
		LiveObjectPtrDetail::CheckAccess(TLifetime::IsAlive(m_id));
	}
#endif
	return m_ptr;
}

template <typename T, typename TLifetime>
bool LiveObjectPtr<T, TLifetime>::operator==(const T* object) const
{
	if (object == nullptr)
	{
		return !IsValid();
	}
	return m_ptr == object && IsValid();
}
