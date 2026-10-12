#include <Services/SelectionBoundsDisplay.h>

#include <UI/Framework/EditorSelection.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/RenderComponent.h>

#include <algorithm>

namespace
{
	void SetDrawBounds(const GameObjectPtr& gameObject, bool draw)
	{
		// Deleted objects are skipped; their handles are lifetime-checked
		if (gameObject == nullptr)
		{
			return;
		}

		GameObjectComponentPtr<RenderComponent> renderComponent = gameObject->GetComponent<RenderComponent>();
		if (renderComponent != nullptr)
		{
			renderComponent->SetDrawMeshBounds(draw);
		}
	}
}

namespace Editor
{
	SelectionBoundsDisplay::SelectionBoundsDisplay(EditorEvents& events)
		: m_selectionChanged(events.Subscribe<SelectionChangedEvent>([this](const SelectionChangedEvent& event)
			{
				OnSelectionChanged(event.Selection.GetAll());
			}))
	{
	}

	void SelectionBoundsDisplay::OnSelectionChanged(const std::vector<GameObjectPtr>& selection)
	{
		for (const GameObjectPtr& gameObject : m_shown)
		{
			if (std::find(selection.begin(), selection.end(), gameObject) == selection.end())
			{
				SetDrawBounds(gameObject, false);
			}
		}

		for (const GameObjectPtr& gameObject : selection)
		{
			if (std::find(m_shown.begin(), m_shown.end(), gameObject) == m_shown.end())
			{
				SetDrawBounds(gameObject, true);
			}
		}

		m_shown = selection;
	}
}
