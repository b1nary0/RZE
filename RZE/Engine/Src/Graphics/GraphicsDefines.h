#pragma once

class RenderObject;
class LightObject;

class LightObjectPtr
{
	friend class RenderEngine;

public:
	LightObjectPtr() = default;
	LightObjectPtr(const LightObjectPtr&) = default;
	~LightObjectPtr() = default;

	LightObjectPtr(LightObjectPtr&& other)
	{
		m_ptr = other.m_ptr;
		other.m_ptr = nullptr;
	}
	
	LightObject* operator->()
	{
		return m_ptr;
	}
	LightObjectPtr& operator=(const LightObjectPtr&) = default;

	bool operator==(const LightObjectPtr& other)
	{
		return m_ptr == other.m_ptr;
	}

	bool operator==(const LightObjectPtr* other)
	{
		return this == other;
	}

	bool operator!=(const LightObjectPtr& other)
	{
		return !(*this == other);
	}

	bool operator!=(const std::nullptr_t null)
	{
		return this->m_ptr != null;
	}

private:
	LightObject* m_ptr = nullptr;
};

class RenderObjectPtr
{
	friend class RenderEngine;

public:
	RenderObjectPtr() = default;
	RenderObjectPtr(const RenderObjectPtr&) = default;
	~RenderObjectPtr() = default;

	RenderObjectPtr(RenderObjectPtr&& other)
	{
		m_ptr = other.m_ptr;
		other.m_ptr = nullptr;
	}

	RenderObject* operator->()
	{
		return m_ptr;
	}

	RenderObjectPtr& operator=(const RenderObjectPtr&) = default;

	bool operator==(const RenderObjectPtr& other)
	{
		return m_ptr == other.m_ptr;
	}

	bool operator==(const RenderObjectPtr* other)
	{
		return this == other;
	}

	bool operator!=(const RenderObjectPtr& other)
	{
		return !(*this == other);
	}

	bool operator!=(const std::nullptr_t null)
	{
		return this->m_ptr != null;
	}

private:
	RenderObject* m_ptr = nullptr;
};