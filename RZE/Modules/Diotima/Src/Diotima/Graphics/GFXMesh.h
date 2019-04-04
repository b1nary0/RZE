#pragma once

#include <unordered_map>
#include <vector>

#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/PrimitiveDefs.h>

namespace Diotima
{
	class GFXMaterial;

	struct GFXVertex
	{
		Vector3D Position;
		Vector3D Normal;
		Vector3D Tangent;
		Vector2D UVData;
	};

	class GFXMesh
	{
	public:
		GFXMesh();
		~GFXMesh();

		const GFXMaterial& GetMaterial() { return *mMaterial; }
		void SetMaterial(GFXMaterial* material);

		// #TODO::#TEMPHACK(Josh::Mashing this in here to avoid sending in engine-side concept of vertex. They will share the same layout for now
		//                        so until a proper command layer is implemented (soontm) this hack is going to keep things running for the greater good.
		//                        This void* will come in from engine as MeshVertex and be cast to GFXVertex.)
		void Allocate(const std::vector<void*>& vertexData, const std::vector<U32>& indices);
		size_t GetIndexCount() const { return mNumIndices; }

	private:
		GFXMaterial* mMaterial;

		size_t mNumIndices;
	};
}
