#pragma once

#include <Utils/DebugUtils/DebugConfig.h>

// Render stats only
#ifdef RZE_RENDER_STATS

#include <Utils/PrimitiveDefs.h>

#include <array>

namespace Rendering
{
	// What Renderer's set calls bind, for counting the binds that change nothing (StateTracker). Each kind has its own
	// slots: the pipeline stage a constant buffer or texture is bound to makes a different kind.
	enum class EStateKind : U8
	{
		Targets,
		VertexShader,
		PixelShader,
		ComputeShader,
		InputLayout,
		Topology,
		Rasterizer,
		Blend,
		Depth,
		Viewport,
		ConstantBufferVS,
		ConstantBufferPS,
		ConstantBufferCS,
		TexturePS,
		TextureCS,
		UnorderedAccess,
		VertexBuffer,
		IndexBuffer,

		Count
	};

	// What the counts are split by
	enum class EStateCategory : U8
	{
		Targets,
		Shaders,
		// Input layout, topology, rasterizer, blend, depth and viewport
		FixedState,
		ConstantBuffers,
		// Shader resource views and unordered access views
		Textures,
		VertexBuffers,
		IndexBuffers,

		Count
	};

	constexpr U32 k_stateKindCount = static_cast<U32>(EStateKind::Count);
	constexpr U32 k_stateCategoryCount = static_cast<U32>(EStateCategory::Count);

	EStateCategory GetStateCategory(EStateKind kind);
	const char* GetStateCategoryName(EStateCategory category);

	// What a slot is bound to: the object's address, which says whether it's the same object and nothing else, plus
	// what else the bind chooses (a depth slice, a mip, an enum's value). Nothing in it is read through, so it has no GPU.
	struct StateBinding
	{
		const void* Object = nullptr;
		// A second object bound by the same call, e.g. the depth buffer that goes with a colour target
		const void* Related = nullptr;
		U64 Detail = 0;

		bool operator==(const StateBinding& other) const;
	};

	// Remembers what each slot of each kind was last bound to, to tell a bind that changes the slot from one that repeats
	// what it already had. Slots are 16 per kind, more than the engine uses; a slot past that is never a repeat.
	// What's known is only what was bound since Clear(), i.e. in this pass. The device keeps its state between passes,
	// but a pass can't rely on it: Begin() resets the rasterizer state, other passes (the UI) change state, and the
	// order of the stages isn't fixed. So the first bind of each slot in a pass always counts as a change.
	// An object's address may be reused once it's destroyed, which can make a bind of a new object look like a repeat;
	// the counts are a measure, not a decision about what to bind.
	class StateTracker
	{
	public:
		static constexpr U32 k_slotsPerKind = 16;

	public:
		// Records that the slot is bound to binding. Returns whether it was already, a repeat. Binding other targets
		// clears the textures, which can no longer be assumed bound: D3D11 unbinds a texture that's bound to be read
		// when it's bound to be written.
		bool Bind(EStateKind kind, U32 slot, const StateBinding& binding);

		// Everything: a new pass starts from nothing known
		void Clear();
		// Only the textures (shader resource and unordered access views)
		void ClearTextures();

	private:
		struct Slot
		{
			StateBinding Binding;
			// Whether anything was bound since the slot was cleared
			bool IsKnown = false;
		};

	private:
		void ClearKind(EStateKind kind);

	private:
		std::array<std::array<Slot, k_slotsPerKind>, k_stateKindCount> m_slots;
	};
}

#endif // RZE_RENDER_STATS
