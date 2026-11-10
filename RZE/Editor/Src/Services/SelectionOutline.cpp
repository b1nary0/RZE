#include <Services/SelectionOutline.h>

#include <UI/Framework/EditorSelection.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/RenderComponent.h>

#include <Graphics/RenderObject.h>
#include <Graphics/RenderView.h>

#include <algorithm>
#include <vector>

namespace
{
	// Yellow, after tonemapping
	constexpr float k_colour[3] = { 1.0f, 0.8f, 0.1f };
	// In pixels
	constexpr float k_width = 3.0f;
	// Where the selection is behind the scene: enough to show where it is, without competing with what's in front
	constexpr float k_occludedOpacity = 0.3f;
}

namespace Editor
{
	void ApplySelectionOutline(const EditorSelection& selection, RenderView& view)
	{
		if (selection.IsEmpty())
		{
			return;
		}

		RenderOutline outline;
		outline.Colour = Vector3D(k_colour[0], k_colour[1], k_colour[2]);
		outline.Width = k_width;
		outline.OccludedOpacity = k_occludedOpacity;

		std::vector<GameObject*> subtree;
		for (const GameObjectPtr& gameObject : selection.GetAll())
		{
			// Deleted objects are skipped; their handles are lifetime-checked
			if (gameObject == nullptr)
			{
				continue;
			}

			subtree.clear();
			gameObject->GetSubtree(subtree);
			for (GameObject* member : subtree)
			{
				const GameObjectComponentPtr<RenderComponent> renderComponent = member->GetComponent<RenderComponent>();
				// No render object until it has a mesh
				if (renderComponent != nullptr && renderComponent->GetRenderObject() != nullptr)
				{
					outline.ObjectLiveIDs.push_back(renderComponent->GetRenderObject()->GetLiveID());
				}
			}
		}

		// A selected object's selected descendants are in its subtree too
		std::sort(outline.ObjectLiveIDs.begin(), outline.ObjectLiveIDs.end());
		outline.ObjectLiveIDs.erase(std::unique(outline.ObjectLiveIDs.begin(), outline.ObjectLiveIDs.end()), outline.ObjectLiveIDs.end());

		if (!outline.ObjectLiveIDs.empty())
		{
			view.Frame.Outlines.push_back(std::move(outline));
		}
	}
}
