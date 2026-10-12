#pragma once

class StaticMeshInstance;

namespace Editor
{
	// A read-only tree of a mesh's sub-meshes, each with its material's shader and textures (with previews)
	void DrawMeshMaterialTree(const StaticMeshInstance& mesh);
}
