#include <Rendering/StateTracker.h>

// Render stats only (its header is empty without them)
#ifdef RZE_RENDER_STATS

namespace Rendering
{
	EStateCategory GetStateCategory(EStateKind kind)
	{
		switch (kind)
		{
		case EStateKind::Targets:
			return EStateCategory::Targets;
		case EStateKind::VertexShader:
		case EStateKind::PixelShader:
		case EStateKind::ComputeShader:
			return EStateCategory::Shaders;
		case EStateKind::InputLayout:
		case EStateKind::Topology:
		case EStateKind::Rasterizer:
		case EStateKind::Blend:
		case EStateKind::Depth:
		case EStateKind::Viewport:
			return EStateCategory::FixedState;
		case EStateKind::ConstantBufferVS:
		case EStateKind::ConstantBufferPS:
		case EStateKind::ConstantBufferCS:
			return EStateCategory::ConstantBuffers;
		case EStateKind::TexturePS:
		case EStateKind::TextureCS:
		case EStateKind::UnorderedAccess:
			return EStateCategory::Textures;
		case EStateKind::VertexBuffer:
			return EStateCategory::VertexBuffers;
		case EStateKind::IndexBuffer:
			return EStateCategory::IndexBuffers;
		case EStateKind::Count:
			break;
		}
		return EStateCategory::Count;
	}

	const char* GetStateCategoryName(EStateCategory category)
	{
		switch (category)
		{
		case EStateCategory::Targets:
			return "Targets";
		case EStateCategory::Shaders:
			return "Shaders";
		case EStateCategory::FixedState:
			return "Fixed state";
		case EStateCategory::ConstantBuffers:
			return "Constant buffers";
		case EStateCategory::Textures:
			return "Textures";
		case EStateCategory::VertexBuffers:
			return "Vertex buffers";
		case EStateCategory::IndexBuffers:
			return "Index buffers";
		case EStateCategory::Count:
			break;
		}
		return "";
	}

	bool StateBinding::operator==(const StateBinding& other) const
	{
		return Object == other.Object && Related == other.Related && Detail == other.Detail;
	}

	bool StateTracker::Bind(EStateKind kind, U32 slot, const StateBinding& binding)
	{
		if (slot >= k_slotsPerKind)
		{
			return false;
		}

		Slot& tracked = m_slots[static_cast<U32>(kind)][slot];
		const bool isRepeat = tracked.IsKnown && tracked.Binding == binding;
		tracked.Binding = binding;
		tracked.IsKnown = true;

		if (kind == EStateKind::Targets && !isRepeat)
		{
			ClearTextures();
		}
		return isRepeat;
	}

	void StateTracker::Clear()
	{
		for (U32 kindIndex = 0; kindIndex < k_stateKindCount; ++kindIndex)
		{
			ClearKind(static_cast<EStateKind>(kindIndex));
		}
	}

	void StateTracker::ClearTextures()
	{
		ClearKind(EStateKind::TexturePS);
		ClearKind(EStateKind::TextureCS);
		ClearKind(EStateKind::UnorderedAccess);
	}

	void StateTracker::ClearKind(EStateKind kind)
	{
		for (Slot& slot : m_slots[static_cast<U32>(kind)])
		{
			slot.IsKnown = false;
		}
	}
}

#endif // RZE_RENDER_STATS
