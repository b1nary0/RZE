#pragma once

#include <Utils/PrimitiveDefs.h>

namespace Editor
{
	// Identifies one continuous interaction, such as a gizmo drag, across the commands it pushes each frame, so
	// undo can later treat the whole drag as one step
	using InteractionID = U32;
	constexpr InteractionID k_noInteraction = 0;

	// Turns a control's per-frame "is it being used" into the start and end of each interaction, and gives each
	// interaction a unique ID. Update it every frame the control exists, so no start or end is missed.
	class InteractionTracker
	{
	public:
		InteractionTracker() = default;
		~InteractionTracker() = default;

	public:
		void Update(bool isActive);

		bool IsActive() const { return m_isActive; }
		// True only on the frame the interaction started or ended
		bool HasBegun() const { return m_isActive && !m_wasActive; }
		bool HasEnded() const { return !m_isActive && m_wasActive; }

		// The current interaction, or the one that just ended; k_noInteraction before the first
		InteractionID GetID() const { return m_id; }

		// Forgets any interaction in progress without reporting it as ended, e.g. when the scene it was editing unloads
		void Reset();

	private:
		bool m_isActive = false;
		bool m_wasActive = false;
		InteractionID m_id = k_noInteraction;
	};
}
