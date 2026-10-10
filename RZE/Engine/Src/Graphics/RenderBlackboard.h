#pragma once

#include <Graphics/RenderData/RenderStageData.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/PrimitiveDefs.h>

#include <memory>
#include <unordered_map>

// Typed storage for data contracts (RenderStageData). Holds at most one value per contract type.
//
// A RenderView has two. Its frame blackboard carries the contracts stages pass each other while rendering
// the view; stages reach it only through their ports (RenderPorts.h), and BeginFrame() clears it before each
// render. Its persistent blackboard (RenderContext::Persistent()) keeps values for as long as the view exists.
class RenderBlackboard
{
public:
	RenderBlackboard() = default;
	RenderBlackboard(const RenderBlackboard&) = delete;
	RenderBlackboard& operator=(const RenderBlackboard&) = delete;

public:
	// Drops every value published so far. Slots are kept, so steady state doesn't allocate.
	void BeginFrame() { ++m_generation; }

	template <typename T>
	void Publish(const T& value)
	{
		static_assert(IsRenderStageData<T>, "Render stage data must derive from RenderStageData<T>");

		std::unique_ptr<ISlot>& slot = m_slots[T::TypeId()];
		if (slot == nullptr)
		{
			slot = std::make_unique<Slot<T>>();
		}

		static_cast<Slot<T>*>(slot.get())->Value = value;
		slot->Generation = m_generation;
	}

	template <typename T>
	const T* TryGet() const
	{
		return Find<T>();
	}

	template <typename T>
	const T& Get() const
	{
		const T* const value = TryGet<T>();
		AssertMsg(value != nullptr, "Render stage data was read before any stage published it");
		return *value;
	}

	// Returns the value, first publishing init() if there isn't one. For state kept across frames.
	template <typename T, typename TInit>
	T& GetOrCreate(TInit&& init)
	{
		if (Find<T>() == nullptr)
		{
			Publish<T>(init());
		}

		// This blackboard owns the value, so handing out mutable access is safe here
		return *const_cast<T*>(Find<T>());
	}

private:
	struct ISlot
	{
		virtual ~ISlot() = default;
		U32 Generation = 0;
	};

	template <typename T>
	struct Slot : ISlot
	{
		T Value;
	};

	template <typename T>
	const T* Find() const
	{
		static_assert(IsRenderStageData<T>, "Render stage data must derive from RenderStageData<T>");

		const auto iter = m_slots.find(T::TypeId());
		if (iter == m_slots.end() || iter->second->Generation != m_generation)
		{
			return nullptr;
		}

		return &static_cast<const Slot<T>*>(iter->second.get())->Value;
	}

private:
	std::unordered_map<RenderDataTypeId, std::unique_ptr<ISlot>> m_slots;
	// Starts above every slot's initial generation so nothing reads as published before it is
	U32 m_generation = 1;
};
