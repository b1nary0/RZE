#pragma once

// @TODO When GameObjectPtr doesnt live in GameObject.h remove this include
#include <Game/World/GameObject/GameObjectDefinitions.h>
#include <Game/World/GameObject/ObjectGUID.h>

#include <RapidJSON/fwd.h>

#include <Utils/Math/Vector3D.h>

#include <functional>
#include <memory>
#include <unordered_map>

class RenderScene;

class GameScene final
{
public:
	GameScene();
	virtual ~GameScene();

public:
	virtual void Start();
	virtual void Update();
	virtual void ShutDown();

	void Initialize();

	void NewScene();
	void Serialize(const Filepath& filePath);
	// Synchronous load. See SceneLoadOperation for the asynchronous equivalent.
	void Deserialize(const Filepath& filePath);
	void Unload();

	static const Filepath& GetDefaultScenePath();

	// Reads and parses a scene file without touching any scene state. Safe to call from any thread.
	static bool ParseSceneFile(const Filepath& filePath, rapidjson::Document& outDocument, std::string& outError);

	// Main thread. Creates a game object from its entry in the scene format (SceneFormat), under parent if there is
	// one, and adds it to the scene. Its parent link in the entry is ignored; GameObjectLoader resolves those.
	GameObjectPtr DeserializeGameObject(const rapidjson::Value& entry, GameObjectPtr parent);

	void SetCurrentScenePath(const Filepath& filePath) { mCurrentScenePath = filePath; }
	const Filepath& GetCurrentScenePath() const { return mCurrentScenePath; }
	
	// The first object in the scene with this name, or null. Names aren't unique (objects are identified by
	// ObjectGUID), so this is for convenience, e.g. finding a scene's only "Camera".
	GameObjectPtr FindGameObjectByName(const std::string& name);
	// Null if no object in the scene has guid
	GameObjectPtr FindGameObjectByGUID(ObjectGUID guid);
	GameObjectPtr AddGameObject(const std::string& name);
	void AddGameObject(const GameObjectPtr& gameObject);
	
	void ForEachGameObject(Functor<void, GameObjectPtr> func);
	// Takes gameObject and all its descendants out of the scene now, and destroys them at the start of the next
	// Update. Works on objects already out of the scene too. Keeps the order of the objects left.
	void RemoveGameObject(GameObjectPtr& gameObject);

	// Called whenever an object is added to the scene or removed from it, for tools that track that. Empty by default.
	void SetObjectsChangedCallback(std::function<void()> callback) { m_onObjectsChanged = std::move(callback); }
	
	// @NOTE Creates GameObject with TransformComponent, as all gameobjects have a spatial representation
	GameObjectPtr CreateGameObject();

	// The scene's main camera: the one with CameraComponent::IsMainCamera, which the game renders through. Null if
	// the scene has none. Cached by SetMainCamera, so it's cheap.
	GameObjectPtr GetMainCamera() const;
	// Makes camera (which must have a CameraComponent) the main camera and clears the flag on the one before it.
	// Null leaves the scene without one.
	void SetMainCamera(const GameObjectPtr& camera);
	// After a load, makes sure there's one main camera if there are any cameras: the first flagged one, or the first
	// camera in scenes saved before the flag existed
	void ResolveMainCamera();

	// Whether the main camera drives the main view. The Editor turns this off, since its own camera does; the main
	// camera's exposure still applies.
	void SetMainCameraDrivesView(bool drivesView) { m_mainCameraDrivesView = drivesView; }
	bool DoesMainCameraDriveView() const { return m_mainCameraDrivesView; }

	// What the scene's components draw: their render objects, lights and debug lines. RenderEngine draws the active
	// scene's in the main view; any scene's can be rendered into another view (RenderEngine::RenderSecondaryView).
	RenderScene& GetRenderScene() const { return *m_renderScene; }

	// Center of the bounding box around every rendered root object's position. Uses positions rather
	// than mesh bounds so it's valid while meshes are still streaming in. Returns the origin if nothing is rendered.
	Vector3D CalculateSceneCenter() const;

private:
	// @NOTE Creates GameObject with no components. Just used for load code.
	std::unique_ptr<GameObject> CreateGameObjectNoComponents(ObjectGUID guid);

	// The GUID saved in a loaded object's data ("id") if it's valid and not taken, otherwise a new one
	ObjectGUID ChooseGUID(const rapidjson::Value& data) const;
	// A new GUID that no object in the scene has
	ObjectGUID GenerateFreeGUID() const;

	void AddGameObject(std::unique_ptr<GameObject>&& gameObject);

	// When the main camera is gone (it left the scene, removed or set aside by a tool, or lost its CameraComponent),
	// the first camera still in the scene takes over, if there is one
	void ReplaceMainCameraIfGone();
	bool IsMainCameraGone() const;

	void ProcessObjectRemoveDeferrals();

private:
	Filepath mCurrentScenePath;

	// First, so it outlives the objects whose components draw into it
	std::unique_ptr<RenderScene> m_renderScene;

	// #TODO Implement sparse array to make this faster
	std::vector<std::unique_ptr<GameObject>> m_objectRegistry;

	std::vector<GameObjectPtr> m_objectsToRemove;

	// Every object in m_objectRegistry by GUID, including ones waiting to be removed; GUIDs never change, so this
	// only changes when objects are added or erased
	std::unordered_map<ObjectGUID, GameObject*> m_objectsByGUID;

	// Set only by SetMainCamera, alongside the camera's flag
	GameObjectPtr m_mainCamera;
	bool m_mainCameraDrivesView = true;

	std::function<void()> m_onObjectsChanged;
};