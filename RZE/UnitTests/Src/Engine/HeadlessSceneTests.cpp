#include <doctest/doctest.h>

#include <RZE.h>

#include <Game/GameScene.h>
#include <Game/HeadlessScene.h>
#include <Game/World/GameObject/GameObject.h>

// Loads and preloads need the engine's AsyncOperationManager running, so they're checked in the editor instead

TEST_CASE("A headless scene destroys what's removed from it straight away, since it's never updated")
{
	HeadlessScene scene;

	GameObjectPtr staged = scene.AddGameObject("Staged");
	GameObjectPtr child = scene.AddGameObject("Child");
	child->AttachTo(staged);
	const GameObjectPtr stagedHandle = staged;
	const GameObjectPtr childHandle = child;

	scene.RemoveGameObject(staged);

	CHECK(staged == nullptr);
	CHECK(stagedHandle == nullptr);
	CHECK(childHandle == nullptr);
	CHECK(scene.GetScene().FindGameObjectByName("Staged") == nullptr);
}

TEST_CASE("Unloading a headless scene empties it, and it can be used again")
{
	HeadlessScene scene;

	const GameObjectPtr first = scene.AddGameObject("First");
	CHECK_FALSE(scene.IsLoading());

	scene.Unload();
	CHECK(first == nullptr);
	CHECK(scene.GetScene().FindGameObjectByName("First") == nullptr);

	const GameObjectPtr second = scene.AddGameObject("Second");
	CHECK(&second->GetScene() == &scene.GetScene());
}
