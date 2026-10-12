#include <UI/Framework/InteractionTracker.h>

namespace
{
	// Shared by every tracker, so IDs are unique across controls
	Editor::InteractionID s_nextInteractionID = Editor::k_noInteraction + 1;
}

namespace Editor
{
	InteractionID NewInteractionID()
	{
		return s_nextInteractionID++;
	}

	void InteractionTracker::Update(bool isActive)
	{
		m_wasActive = m_isActive;
		m_isActive = isActive;

		if (HasBegun())
		{
			m_id = NewInteractionID();
		}
	}

	void InteractionTracker::Reset()
	{
		m_isActive = false;
		m_wasActive = false;
	}
}
