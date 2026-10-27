#pragma once

#include <EngineCore/Resources/ResourceHandler.h>

#include <type_traits>
#include <utility>

// A handle to a resource of a known type, with the resource resolved once, for code that loads something up front and
// uses it often (a render stage's shaders, a material's textures). Holding it keeps the resource loaded; Reset or
// destroying it lets go. The synchronous counterpart of PendingResource. Move-only, like ResourceHandle. Main thread
// only. TResource can be incomplete where one is only declared, e.g. a member in a header.
template <typename TResource>
class TypedResourceHandle
{
public:
	TypedResourceHandle() = default;
	// Empty if handle is
	explicit TypedResourceHandle(ResourceHandle&& handle);
	// Shares the handle's resource
	explicit TypedResourceHandle(const ResourceHandle& handle);

	// Moving leaves other empty
	TypedResourceHandle(TypedResourceHandle&& other);
	TypedResourceHandle& operator=(TypedResourceHandle&& other);
	TypedResourceHandle(const TypedResourceHandle&) = delete;
	TypedResourceHandle& operator=(const TypedResourceHandle&) = delete;

public:
	bool IsValid() const { return m_resource != nullptr; }
	// Null when empty
	const TResource* Get() const { return m_resource; }
	const TResource* operator->() const { return m_resource; }
	const ResourceHandle& GetHandle() const { return m_handle; }

	// Lets go of the resource
	void Reset();

private:
	ResourceHandle m_handle;
	const TResource* m_resource = nullptr;
};

template <typename TResource>
TypedResourceHandle<TResource>::TypedResourceHandle(ResourceHandle&& handle)
	: m_handle(std::move(handle))
	, m_resource(static_cast<const TResource*>(m_handle.GetResource()))
{
	static_assert(std::is_base_of_v<IResource, TResource>);
}

template <typename TResource>
TypedResourceHandle<TResource>::TypedResourceHandle(const ResourceHandle& handle)
{
	static_assert(std::is_base_of_v<IResource, TResource>);
	m_handle = handle;
	m_resource = static_cast<const TResource*>(m_handle.GetResource());
}

template <typename TResource>
TypedResourceHandle<TResource>::TypedResourceHandle(TypedResourceHandle&& other)
	: m_handle(std::move(other.m_handle))
	, m_resource(std::exchange(other.m_resource, nullptr))
{
}

template <typename TResource>
TypedResourceHandle<TResource>& TypedResourceHandle<TResource>::operator=(TypedResourceHandle&& other)
{
	if (this != &other)
	{
		m_handle = std::move(other.m_handle);
		m_resource = std::exchange(other.m_resource, nullptr);
	}
	return *this;
}

template <typename TResource>
void TypedResourceHandle<TResource>::Reset()
{
	m_handle = ResourceHandle::EmptyHandle();
	m_resource = nullptr;
}
