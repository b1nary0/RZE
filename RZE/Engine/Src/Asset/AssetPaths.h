#pragma once

#include <Utils/Platform/Filepath.h>

#include <string>

// Where each kind of asset lives in the project, and the extension its files have. One place for the code that writes
// assets (the asset burner), loads them (components, scenes) and lists them (the editor's file browser).
namespace AssetPaths
{
	// Scene files
	const Filepath& GetSceneFolder();
	constexpr char k_sceneExtension[] = ".scene";
	// A scene in GetSceneFolder by its file name, e.g. "Sponza.scene"
	Filepath GetScene(const std::string& fileName);
	// The inverse of GetScene: scene's path within GetSceneFolder, e.g. "Sponza.scene" or "Tests\Lights.scene". Empty
	// if scene isn't in that folder.
	std::string GetSceneName(const Filepath& scene);

	// Mesh assets, which the asset burner writes (MeshAssetWriter)
	const Filepath& GetMeshFolder();
	constexpr char k_meshExtension[] = ".meshasset";

	// Material assets, which the asset burner writes (MaterialAssetWriter), a folder per mesh
	const Filepath& GetMaterialFolder();
	constexpr char k_materialExtension[] = ".materialasset";
}
