#include <doctest/doctest.h>

#include <Asset/AssetPaths.h>

TEST_CASE("IsScene and IsMesh go by the extension, ignoring case")
{
	CHECK(AssetPaths::IsScene(Filepath("Assets/Scenes/Sponza.scene")));
	CHECK(AssetPaths::IsScene(Filepath("Assets/Scenes/Tests/Lights.SCENE")));
	CHECK_FALSE(AssetPaths::IsScene(Filepath("ProjectData/Mesh/Panzer.meshasset")));
	CHECK_FALSE(AssetPaths::IsScene(Filepath("Assets/Scenes/Sponza.scene.bak")));

	CHECK(AssetPaths::IsMesh(Filepath("ProjectData/Mesh/Panzer.meshasset")));
	CHECK(AssetPaths::IsMesh(Filepath("ProjectData/Mesh/Panzer.MeshAsset")));
	CHECK_FALSE(AssetPaths::IsMesh(Filepath("ProjectData/Material/Panzer/Hull.materialasset")));
	// No extension at all, e.g. a folder
	CHECK_FALSE(AssetPaths::IsMesh(Filepath("ProjectData/Mesh")));
}
