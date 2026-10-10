#pragma once

#include <type_traits>

// Identifies a data contract type at runtime without RTTI
using RenderDataTypeId = const void*;

// Base for data contracts: the typed packets render stages hand to each other through a RenderView's
// blackboard. Derive with CRTP and give the contract a name for validation and log messages:
//
//	struct ShadowMapData : RenderStageData<ShadowMapData>
//	{
//		static constexpr const char* k_name = "ShadowMapData";
//		Rendering::TextureBuffer2DHandle ShadowMap;
//	};
//
// Contracts should be cheap to copy (handles and plain values). Put each in its own header so
// consumers depend on the data rather than on the stage that produces it.
template <typename TDerived>
struct RenderStageData
{
	static RenderDataTypeId TypeId()
	{
		// One instance per contract type, so its address is a unique ID
		static const char s_typeTag = 0;
		return &s_typeTag;
	}

	static const char* Name()
	{
		return TDerived::k_name;
	}
};

template <typename T>
constexpr bool IsRenderStageData = std::is_base_of_v<RenderStageData<T>, T>;
