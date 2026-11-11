#include <doctest/doctest.h>

#include <RZE.h>

#include <Game/GameScene.h>
#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/LightComponent.h>
#include <Game/World/GameObjectComponents/ReflectionProbeComponent.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Graphics/RenderScene.h>

#include <string>
#include <vector>

namespace
{
	// What the lifecycle components went through, in order, as "<name> <step>"
	std::vector<std::string>& Events()
	{
		static std::vector<std::string> s_events;
		return s_events;
	}

	// Records each step it's taken through, and keeps a render object (without a mesh) in its scene while it's in it,
	// as RenderComponent does with one
	template <typename TComponentType>
	class LifecycleComponent : public GameObjectComponent<TComponentType>
	{
	public:
		~LifecycleComponent() override;

		void Initialize() override;
		void Uninitialize() override;
		void OnAddToScene() override;
		void OnRemoveFromScene() override;

		// Set before the component is added, as the name its events are recorded under
		static std::string s_name;

	private:
		void Record(const char* step) const;

	private:
		RenderScene* m_renderScene = nullptr;
		RenderObjectPtr m_renderObject;
	};

	template <typename TComponentType>
	std::string LifecycleComponent<TComponentType>::s_name;

	template <typename TComponentType>
	LifecycleComponent<TComponentType>::~LifecycleComponent()
	{
		Record("Destroyed");
	}

	template <typename TComponentType>
	void LifecycleComponent<TComponentType>::Initialize()
	{
		m_renderScene = &this->GetOwner()->GetScene().GetRenderScene();
		Record("Initialize");
	}

	template <typename TComponentType>
	void LifecycleComponent<TComponentType>::Uninitialize()
	{
		// The transform, added first, is uninitialized last, so it's still there
		Record(this->GetOwner()->template GetComponent<TransformComponent>() != nullptr ? "Uninitialize" : "Uninitialize without transform");
	}

	template <typename TComponentType>
	void LifecycleComponent<TComponentType>::OnAddToScene()
	{
		m_renderObject = m_renderScene->CreateRenderObject(nullptr);
		Record("OnAddToScene");
	}

	template <typename TComponentType>
	void LifecycleComponent<TComponentType>::OnRemoveFromScene()
	{
		m_renderScene->DestroyRenderObject(m_renderObject);
		Record("OnRemoveFromScene");
	}

	template <typename TComponentType>
	void LifecycleComponent<TComponentType>::Record(const char* step) const
	{
		Events().push_back(s_name + " " + step);
	}

	// Two types, since an object takes one component of each
	class FirstLifecycleComponent final : public LifecycleComponent<FirstLifecycleComponent>
	{
	};

	class SecondLifecycleComponent final : public LifecycleComponent<SecondLifecycleComponent>
	{
	};

	struct LifecycleScene
	{
		GameScene Scene;

		LifecycleScene() { Events().clear(); }
		~LifecycleScene() { Scene.Unload(); }

		bool IsRenderSceneEmpty() const
		{
			const RenderScene& renderScene = Scene.GetRenderScene();
			return renderScene.GetRenderObjects().empty() && renderScene.GetLightObjects().empty() && renderScene.GetReflectionProbes().empty();
		}
	};
}

TEST_CASE_FIXTURE(LifecycleScene, "Removing a component takes it out of the scene, then uninitializes it, then destroys it")
{
	GameObjectPtr object = Scene.AddGameObject("Object");
	FirstLifecycleComponent::s_name = "First";
	object->AddComponent<FirstLifecycleComponent>();
	REQUIRE(Scene.GetRenderScene().GetRenderObjects().size() == 1);
	Events().clear();

	object->RemoveComponent<FirstLifecycleComponent>();

	CHECK(Events() == std::vector<std::string>{ "First OnRemoveFromScene", "First Uninitialize", "First Destroyed" });
	CHECK(IsRenderSceneEmpty());
}

TEST_CASE_FIXTURE(LifecycleScene, "An object's components are uninitialized newest first, all before any is destroyed")
{
	GameObjectPtr object = Scene.AddGameObject("Object");
	FirstLifecycleComponent::s_name = "First";
	SecondLifecycleComponent::s_name = "Second";
	object->AddComponent<FirstLifecycleComponent>();
	object->AddComponent<SecondLifecycleComponent>();
	Events().clear();

	Scene.Unload();

	CHECK(Events() == std::vector<std::string>{
		"First OnRemoveFromScene", "Second OnRemoveFromScene",
		"Second Uninitialize", "First Uninitialize",
		"First Destroyed", "Second Destroyed" });
}

TEST_CASE_FIXTURE(LifecycleScene, "Unloading a scene takes every object out of it before uninitializing any")
{
	GameObjectPtr parent = Scene.AddGameObject("Parent");
	GameObjectPtr child = Scene.AddGameObject("Child");
	child->AttachTo(parent);
	FirstLifecycleComponent::s_name = "Parent";
	parent->AddComponent<FirstLifecycleComponent>();
	FirstLifecycleComponent::s_name = "Child";
	child->AddComponent<FirstLifecycleComponent>();
	Events().clear();

	// The name is read as each records, so both now record as "Both"; what's checked is that both leave the scene
	// before either is uninitialized. Each object then destroys its components as it's uninitialized.
	FirstLifecycleComponent::s_name = "Both";
	Scene.Unload();

	CHECK(Events() == std::vector<std::string>{
		"Both OnRemoveFromScene", "Both OnRemoveFromScene",
		"Both Uninitialize", "Both Destroyed",
		"Both Uninitialize", "Both Destroyed" });
	CHECK(Scene.FindGameObjectByName("Parent") == nullptr);
	CHECK(Scene.FindGameObjectByName("Child") == nullptr);
}

TEST_CASE_FIXTURE(LifecycleScene, "Unloading a scene empties its render scene: render objects, lights and probes")
{
	GameObjectPtr object = Scene.AddGameObject("Object");
	FirstLifecycleComponent::s_name = "First";
	object->AddComponent<FirstLifecycleComponent>();
	Scene.AddGameObject("Light")->AddComponent<LightComponent>();
	Scene.AddGameObject("Probe")->AddComponent<ReflectionProbeComponent>();
	REQUIRE(Scene.GetRenderScene().GetRenderObjects().size() == 1);
	REQUIRE(Scene.GetRenderScene().GetLightObjects().size() == 1);
	REQUIRE(Scene.GetRenderScene().GetReflectionProbes().size() == 1);

	Scene.Unload();

	CHECK(IsRenderSceneEmpty());
}

TEST_CASE_FIXTURE(LifecycleScene, "An object set aside out of the scene is uninitialized once when the scene unloads")
{
	GameObjectPtr object = Scene.AddGameObject("SetAside");
	FirstLifecycleComponent::s_name = "First";
	object->AddComponent<FirstLifecycleComponent>();
	object->OnRemoveFromScene();
	Events().clear();

	Scene.Unload();

	CHECK(Events() == std::vector<std::string>{ "First Uninitialize", "First Destroyed" });
	CHECK(IsRenderSceneEmpty());
}

TEST_CASE_FIXTURE(LifecycleScene, "A removed object leaves the scene at once, and is uninitialized when its removal is processed")
{
	GameObjectPtr object = Scene.AddGameObject("Removed");
	FirstLifecycleComponent::s_name = "First";
	object->AddComponent<FirstLifecycleComponent>();
	Events().clear();

	Scene.RemoveGameObject(object);
	CHECK(Events() == std::vector<std::string>{ "First OnRemoveFromScene" });
	CHECK(IsRenderSceneEmpty());

	// What the next Update starts with
	Scene.ProcessObjectRemoveDeferrals();
	CHECK(Events() == std::vector<std::string>{ "First OnRemoveFromScene", "First Uninitialize", "First Destroyed" });
}
