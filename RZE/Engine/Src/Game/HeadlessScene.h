#pragma once

#include <EngineCore/Resources/ResourceDependencyList.h>

#include <Game/World/GameObject/GameObjectDefinitions.h>

#include <memory>
#include <string>
#include <vector>

class AsyncOperation;
class Filepath;
class GameScene;
class ViewCapture;
struct RenderCamera;

// A GameScene of its own for rendering off-screen, e.g. thumbnails, while the active scene carries on as normal.
// Nothing in it reaches the active scene, the main view or the editor: its loads run in the background without
// blocking anything, and its objects draw only into its own RenderScene, which is only rendered when it's captured.
//
// It's never updated. Render objects take their transform when they're made and lights their direction when they're
// added, which is all a still capture needs. Objects removed from it are destroyed straight away.
//
// Its loads and preloads are Background and Hidden operations: whoever uses the scene reports on its own progress.
//
// Main thread only. Unload it before the engine shuts down, since it holds resources and render data.
class HeadlessScene
{
public:
	HeadlessScene();
	// Unloads
	~HeadlessScene();

	HeadlessScene(const HeadlessScene&) = delete;
	HeadlessScene& operator=(const HeadlessScene&) = delete;

public:
	// What's been loaded and added. A Load while another is still running starts a new scene, so don't keep this
	// across loads.
	GameScene& GetScene() { return *m_scene; }

	// Replaces what's in the scene with the scene file at scenePath (an invalid one loads the default scene), loading
	// it in the background (SceneLoadOperation). Cancels a load that's still running. The scene is ready to capture
	// once the operation succeeds; if it fails, the scene is left empty.
	std::shared_ptr<AsyncOperation> Load(const Filepath& scenePath);

	// Loads a resource, and whatever it depends on, in the background, and keeps it loaded until Unload. An object
	// added afterwards that uses it (e.g. through RenderComponent::LoadMesh) then doesn't stall the main thread.
	template <class TResource>
	std::shared_ptr<AsyncOperation> Preload(const Filepath& resourcePath);
	std::shared_ptr<AsyncOperation> Preload(ResourceDependencyList resources);

	// Whether a Load or Preload is still running
	bool IsLoading() const;
	// Lets go of what's been preloaded, cancelling preloads still running. What the scene's objects use stays loaded
	// through them, e.g. a mesh once a RenderComponent has it.
	void ReleasePreloaded();

	GameObjectPtr AddGameObject(const std::string& name);
	// Takes object and its descendants out of the scene and destroys them, and clears object
	void RemoveGameObject(GameObjectPtr& object);

	// Renders the scene from camera into capture as part of the current frame, and saves it to filepath
	// (ViewCapture::Capture)
	std::shared_ptr<AsyncOperation> Capture(ViewCapture& capture, const RenderCamera& camera, float exposureCompensation, const Filepath& filepath);

	// Cancels any load that's still running, destroys everything in the scene, lets go of what was preloaded, and
	// releases the scene's render state (e.g. its shadow map)
	void Unload();

private:
	// Cancels the scene load if it's still running
	void CancelLoad();

private:
	// Shared with its load, which keeps it alive until it finishes
	std::shared_ptr<GameScene> m_scene;

	std::shared_ptr<AsyncOperation> m_load;
	// Each holds what it loaded until it's released
	std::vector<std::shared_ptr<AsyncOperation>> m_preloads;
};

template <class TResource>
std::shared_ptr<AsyncOperation> HeadlessScene::Preload(const Filepath& resourcePath)
{
	ResourceDependencyList resources;
	resources.Add<TResource>(resourcePath.GetRelativePath());
	return Preload(std::move(resources));
}
