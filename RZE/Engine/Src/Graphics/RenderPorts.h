#pragma once

#include <Graphics/RenderContext.h>

#include <Utils/DebugUtils/Debug.h>

class RenderStageBuilder;

// Ports are how a render stage reaches the data contracts it declared. RenderStageBuilder hands them out
// in IRenderStage::Setup(), and the stage keeps them as members, so a stage's header lists its inputs and
// outputs, and it can't touch a contract it didn't declare.
//
//	m_shadowMap = builder.Reads<ShadowMapData>();				// RenderInput<ShadowMapData>
//	const ShadowMapData& shadowMap = m_shadowMap.Get(context);
//
// A default-constructed port hasn't been declared, and asserts when used.

// Data another stage writes. From Reads<T>() and ReadsOptional<T>().
template <typename T>
class RenderInput
{
	friend class RenderStageBuilder;

public:
	RenderInput() = default;

	const T& Get(const RenderContext& context) const
	{
		AssertMsg(m_isDeclared, "Render port used without being declared in Setup()");
		return context.FrameData().Get<T>();
	}

	// For ReadsOptional: nullptr when no stage published the data for this view
	const T* TryGet(const RenderContext& context) const
	{
		AssertMsg(m_isDeclared, "Render port used without being declared in Setup()");
		return context.FrameData().TryGet<T>();
	}

private:
	bool m_isDeclared = false;
};

// Data this stage produces. From Writes<T>().
template <typename T>
class RenderOutput
{
	friend class RenderStageBuilder;

public:
	RenderOutput() = default;

	void Publish(RenderContext& context, const T& value) const
	{
		AssertMsg(m_isDeclared, "Render port used without being declared in Setup()");
		context.FrameData().Publish<T>(value);
	}

private:
	bool m_isDeclared = false;
};

// Data another stage writes that this stage changes in place, e.g. by drawing into its target. From Modifies<T>().
template <typename T>
class RenderInOut
{
	friend class RenderStageBuilder;

public:
	RenderInOut() = default;

	const T& Get(const RenderContext& context) const
	{
		AssertMsg(m_isDeclared, "Render port used without being declared in Setup()");
		return context.FrameData().Get<T>();
	}

	// Replaces the value later stages see
	void Publish(RenderContext& context, const T& value) const
	{
		AssertMsg(m_isDeclared, "Render port used without being declared in Setup()");
		context.FrameData().Publish<T>(value);
	}

private:
	bool m_isDeclared = false;
};
