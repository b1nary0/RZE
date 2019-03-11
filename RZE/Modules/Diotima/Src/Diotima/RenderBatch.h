#pragma once

#include <vector>

#include <Utils/PrimitiveDefs.h>

namespace Diotima
{
	class GFXMesh;

	class RenderBatch
	{
	public:
		RenderBatch();

		void Allocate(const std::vector<GFXMesh*>& meshes);

	public:
		U32 mNumIndices;
	};
}