#include <doctest/doctest.h>

#include <RZE.h>

#include <Services/SelectionOutline.h>
#include <UI/Framework/EditorEvents.h>
#include <UI/Framework/EditorSelection.h>

#include <Game/GameScene.h>
#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/RenderComponent.h>

#include <Graphics/RenderObject.h>
#include <Graphics/RenderView.h>
#include <Graphics/StaticMesh.h>
#include <Graphics/StaticMeshInstance.h>

#include <algorithm>
#include <map>
#include <vector>

using namespace Editor;

namespace
{
	// A mesh with no geometry or slots: enough for a RenderComponent to make a render object, without the renderer
	std::shared_ptr<StaticMeshInstance> MakeEmptyMeshInstance()
	{
		return StaticMeshInstance::Create(StaticMesh::Create("Test", std::vector<StaticMesh::SubMesh>(), std::vector<StaticMesh::Slot>()));
	}

	struct OutlineFixture
	{
		GameScene Scene;
		EditorEvents Events;
		EditorSelection Selection{ Events };
		RenderView View{ ERenderViewKind::Main };

		// Parent (a mesh) has children Child (a mesh) and Empty (none); Other is a root mesh
		GameObjectPtr Parent = AddMeshObject("Parent");
		GameObjectPtr Child = AddMeshObject("Child", Parent);
		GameObjectPtr Empty = AddObject("Empty", Parent);
		GameObjectPtr Other = AddMeshObject("Other");

		~OutlineFixture()
		{
			Scene.Unload();
		}

		GameObjectPtr AddObject(const char* name, const GameObjectPtr& parent = GameObjectPtr())
		{
			GameObjectPtr gameObject = Scene.AddGameObject(name);
			if (parent != nullptr)
			{
				gameObject->AttachTo(parent);
			}
			return gameObject;
		}

		GameObjectPtr AddMeshObject(const char* name, const GameObjectPtr& parent = GameObjectPtr())
		{
			GameObjectPtr gameObject = AddObject(name, parent);
			gameObject->AddComponent<RenderComponent>()->SetMeshInstance(MakeEmptyMeshInstance());
			return gameObject;
		}

		static U64 GetLiveID(const GameObjectPtr& gameObject)
		{
			return gameObject->GetComponent<RenderComponent>()->GetRenderObject()->GetLiveID();
		}

		// The live IDs of the view's one outline, sorted
		std::vector<U64> GetOutlinedIDs() const
		{
			REQUIRE(View.Frame.Outlines.size() == 1);
			return Sorted(GetLiveIDs(View.Frame.Outlines.front()));
		}

		// Each object of the view's one outline, by live ID, with its shape
		std::map<U64, U32> GetShapes() const
		{
			REQUIRE(View.Frame.Outlines.size() == 1);
			std::map<U64, U32> shapes;
			for (const RenderOutlineObject& object : View.Frame.Outlines.front().Objects)
			{
				CHECK(shapes.count(object.LiveID) == 0);
				shapes[object.LiveID] = object.Shape;
			}
			return shapes;
		}

		static std::vector<U64> GetLiveIDs(const RenderOutline& outline)
		{
			std::vector<U64> ids;
			for (const RenderOutlineObject& object : outline.Objects)
			{
				ids.push_back(object.LiveID);
			}
			return ids;
		}

		static std::vector<U64> Sorted(std::vector<U64> ids)
		{
			std::sort(ids.begin(), ids.end());
			return ids;
		}
	};
}

TEST_CASE_FIXTURE(OutlineFixture, "Nothing selected adds no outline, and leaves the view's others")
{
	ApplySelectionOutline(Selection, View);
	CHECK(View.Frame.Outlines.empty());

	View.Frame.Outlines.push_back(RenderOutline());
	ApplySelectionOutline(Selection, View);
	CHECK(View.Frame.Outlines.size() == 1);
}

TEST_CASE_FIXTURE(OutlineFixture, "The selection's outline is added after the view's others")
{
	RenderOutline other;
	other.Objects = { { GetLiveID(Other), 0 } };
	View.Frame.Outlines.push_back(other);
	Selection.Select(Child);

	ApplySelectionOutline(Selection, View);

	REQUIRE(View.Frame.Outlines.size() == 2);
	CHECK(GetLiveIDs(View.Frame.Outlines[0]) == std::vector<U64>{ GetLiveID(Other) });
	CHECK(GetLiveIDs(View.Frame.Outlines[1]) == std::vector<U64>{ GetLiveID(Child) });
}

TEST_CASE_FIXTURE(OutlineFixture, "A selected object is outlined with its descendants, as one outline")
{
	Selection.Select(Parent);

	ApplySelectionOutline(Selection, View);

	CHECK(GetOutlinedIDs() == Sorted({ GetLiveID(Parent), GetLiveID(Child) }));
	CHECK(View.Frame.Outlines.front().Width > 0.0f);
	CHECK(View.Frame.Outlines.front().OccludedOpacity > 0.0f);
}

TEST_CASE_FIXTURE(OutlineFixture, "An object with no mesh outlines nothing")
{
	Selection.Select(Empty);

	ApplySelectionOutline(Selection, View);

	CHECK(View.Frame.Outlines.empty());
}

TEST_CASE_FIXTURE(OutlineFixture, "A selected parent and its selected child are each outlined once")
{
	Selection.SetSelection({ Parent, Child, Other });

	ApplySelectionOutline(Selection, View);

	CHECK(GetShapes() == std::map<U64, U32>{ { GetLiveID(Parent), 0 }, { GetLiveID(Child), 0 }, { GetLiveID(Other), 1 } });
}

TEST_CASE_FIXTURE(OutlineFixture, "Each selected object is a shape of its own with its descendants, in selection order")
{
	Selection.SetSelection({ Other, Parent });

	ApplySelectionOutline(Selection, View);

	CHECK(GetShapes() == std::map<U64, U32>{ { GetLiveID(Other), 0 }, { GetLiveID(Parent), 1 }, { GetLiveID(Child), 1 } });
}

TEST_CASE_FIXTURE(OutlineFixture, "A selected child is in its selected parent's shape, also when selected first")
{
	Selection.SetSelection({ Child, Parent });

	ApplySelectionOutline(Selection, View);

	CHECK(GetShapes() == std::map<U64, U32>{ { GetLiveID(Parent), 0 }, { GetLiveID(Child), 0 } });
}

TEST_CASE_FIXTURE(OutlineFixture, "A selected object with no mesh takes no shape number")
{
	Selection.SetSelection({ Empty, Other });

	ApplySelectionOutline(Selection, View);

	CHECK(GetShapes() == std::map<U64, U32>{ { GetLiveID(Other), 0 } });
}

TEST_CASE_FIXTURE(OutlineFixture, "A selected object that's been destroyed is skipped")
{
	Selection.SetSelection({ Other, Child });
	const U64 childID = GetLiveID(Child);
	Scene.RemoveGameObject(Other);
	Scene.ProcessObjectRemoveDeferrals();

	ApplySelectionOutline(Selection, View);

	CHECK(GetOutlinedIDs() == std::vector<U64>{ childID });
}

TEST_CASE_FIXTURE(OutlineFixture, "A render object made again is outlined by its new ID")
{
	Selection.Select(Other);
	ApplySelectionOutline(Selection, View);
	const U64 oldID = GetOutlinedIDs().front();
	// As the render does, before the next frame's requests
	View.ClearFrameRequests();

	// Without a mesh the render object goes; with one again, it's a new one
	GameObjectComponentPtr<RenderComponent> renderComponent = Other->GetComponent<RenderComponent>();
	renderComponent->SetMeshInstance(nullptr);
	renderComponent->SetMeshInstance(MakeEmptyMeshInstance());
	ApplySelectionOutline(Selection, View);

	CHECK(GetOutlinedIDs() == std::vector<U64>{ GetLiveID(Other) });
	CHECK(GetLiveID(Other) != oldID);
}
