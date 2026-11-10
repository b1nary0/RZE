#include <Services/SelectionOutline.h>

#include <UI/Framework/EditorSelection.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/RenderComponent.h>

#include <Graphics/RenderObject.h>
#include <Graphics/RenderView.h>

#include <algorithm>
#include <functional>
#include <vector>

namespace
{
	// Yellow, after tonemapping
	constexpr float k_colour[3] = { 1.0f, 0.8f, 0.1f };
	// In pixels
	constexpr float k_width = 3.0f;
	// Where the selection is behind the scene: enough to show where it is, without competing with what's in front
	constexpr float k_occludedOpacity = 0.3f;

	// In a selected ancestor's subtree, so outlined as part of that ancestor's shape. selected is sorted (std::less), so
	// a large selection of deep objects isn't searched through for every ancestor of every one, each frame.
	bool HasSelectedAncestor(const std::vector<const GameObject*>& selected, const GameObject& gameObject)
	{
		for (GameObjectPtr ancestor = gameObject.GetParent(); ancestor != nullptr; ancestor = ancestor->GetParent())
		{
			if (std::binary_search(selected.begin(), selected.end(), ancestor.Get(), std::less<const GameObject*>()))
			{
				return true;
			}
		}
		return false;
	}
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

		// Deleted objects are skipped; their handles are lifetime-checked
		std::vector<const GameObject*> selected;
		for (const GameObjectPtr& gameObject : selection.GetAll())
		{
			if (gameObject != nullptr)
			{
				selected.push_back(gameObject.Get());
			}
		}
		std::sort(selected.begin(), selected.end(), std::less<const GameObject*>());

		// Each selected object's subtree is a shape, so each is edged where it's in front of another, as when inside a
		// selected Sponza. Those subtrees don't overlap: a selected object under a selected ancestor is in the ancestor's.
		std::vector<GameObject*> subtree;
		U32 shape = 0;
		for (const GameObjectPtr& gameObject : selection.GetAll())
		{
			if (gameObject == nullptr || HasSelectedAncestor(selected, *gameObject.Get()))
			{
				continue;
			}

			subtree.clear();
			gameObject->GetSubtree(subtree);
			const size_t objectCount = outline.Objects.size();
			for (GameObject* member : subtree)
			{
				const GameObjectComponentPtr<RenderComponent> renderComponent = member->GetComponent<RenderComponent>();
				// No render object until it has a mesh
				if (renderComponent != nullptr && renderComponent->GetRenderObject() != nullptr)
				{
					outline.Objects.push_back({ renderComponent->GetRenderObject()->GetLiveID(), shape });
				}
			}
			// The next shape only once this one has something in it
			if (outline.Objects.size() > objectCount)
			{
				++shape;
			}
		}

		if (!outline.Objects.empty())
		{
			view.Frame.Outlines.push_back(std::move(outline));
		}
	}
}
