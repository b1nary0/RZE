#include <UI/Framework/EditorSelection.h>

#include <UI/Framework/EditorEvents.h>

#include <algorithm>

namespace Editor
{
	EditorSelection::EditorSelection(EditorEvents& events)
		: m_events(events)
	{
	}

	GameObjectPtr EditorSelection::GetPrimary() const
	{
		return m_selection.empty() ? GameObjectPtr() : m_selection.back();
	}

	bool EditorSelection::IsSelected(const GameObjectPtr& gameObject) const
	{
		return std::find(m_selection.begin(), m_selection.end(), gameObject) != m_selection.end();
	}

	void EditorSelection::Clear()
	{
		if (!m_selection.empty())
		{
			m_selection.clear();
			PublishChanged();
		}
	}

	void EditorSelection::Select(GameObjectPtr gameObject)
	{
		SetSelection({ gameObject });
	}

	void EditorSelection::SetSelection(const std::vector<GameObjectPtr>& gameObjects)
	{
		std::vector<GameObjectPtr> selection;
		for (const GameObjectPtr& gameObject : gameObjects)
		{
			if (std::find(selection.begin(), selection.end(), gameObject) == selection.end())
			{
				selection.push_back(gameObject);
			}
		}

		if (selection != m_selection)
		{
			m_selection = std::move(selection);
			PublishChanged();
		}
	}

	void EditorSelection::Toggle(GameObjectPtr gameObject)
	{
		auto it = std::find(m_selection.begin(), m_selection.end(), gameObject);
		if (it == m_selection.end())
		{
			m_selection.push_back(gameObject);
		}
		else
		{
			m_selection.erase(it);
		}

		PublishChanged();
	}

	void EditorSelection::AddToSelection(const std::vector<GameObjectPtr>& gameObjects)
	{
		bool isChanged = false;
		for (const GameObjectPtr& gameObject : gameObjects)
		{
			if (!IsSelected(gameObject))
			{
				m_selection.push_back(gameObject);
				isChanged = true;
			}
		}

		if (isChanged)
		{
			PublishChanged();
		}
	}

	void EditorSelection::PublishChanged()
	{
		m_events.Publish(SelectionChangedEvent{ *this });
	}
}
