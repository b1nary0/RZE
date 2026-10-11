#include <UI/Framework/EditorSelection.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/RenderComponent.h>

#include <algorithm>

namespace
{
	// Selected objects show their bounds through RenderComponent's own toggle so its checkbox stays in sync
	void SetDrawSelectionBounds(GameObjectPtr gameObject, bool draw)
	{
		GameObjectComponentPtr<RenderComponent> renderComponent = gameObject->GetComponent<RenderComponent>();
		if (renderComponent != nullptr)
		{
			renderComponent->SetDrawMeshBounds(draw);
		}
	}
}

namespace Editor
{
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
		for (GameObjectPtr& gameObject : m_selection)
		{
			SetDrawSelectionBounds(gameObject, false);
		}

		m_selection.clear();
	}

	void EditorSelection::Select(GameObjectPtr gameObject)
	{
		Clear();
		Add(gameObject);
	}

	void EditorSelection::SetSelection(const std::vector<GameObjectPtr>& gameObjects)
	{
		Clear();
		AddToSelection(gameObjects);
	}

	void EditorSelection::Toggle(GameObjectPtr gameObject)
	{
		auto it = std::find(m_selection.begin(), m_selection.end(), gameObject);
		if (it == m_selection.end())
		{
			Add(gameObject);
			return;
		}

		SetDrawSelectionBounds(gameObject, false);
		m_selection.erase(it);
	}

	void EditorSelection::AddToSelection(const std::vector<GameObjectPtr>& gameObjects)
	{
		for (const GameObjectPtr& gameObject : gameObjects)
		{
			if (!IsSelected(gameObject))
			{
				Add(gameObject);
			}
		}
	}

	void EditorSelection::Add(GameObjectPtr gameObject)
	{
		m_selection.push_back(gameObject);
		SetDrawSelectionBounds(gameObject, true);
	}
}
