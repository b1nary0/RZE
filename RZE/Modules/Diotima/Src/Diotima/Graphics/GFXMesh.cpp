#include <Diotima/Graphics/GFXMesh.h>

namespace Diotima
{
	GFXMesh::GFXMesh()
	{
		// #TODO(Josh::Move this into an Allocate() function or something - shoud be user/system initiated)
	}

	GFXMesh::~GFXMesh()
	{
	}

	void GFXMesh::SetMaterial(GFXMaterial* material)
	{
		mMaterial = material;
	}

	void GFXMesh::Allocate(const std::vector<void*>& vertexData, const std::vector<U32>& indices)
	{
		std::vector<Vector3D> positions;
		std::vector<Vector3D> normals;
		std::vector<Vector2D> uvCoords;
		std::vector<Vector3D> tangents;

		positions.reserve(vertexData.size());
		normals.reserve(vertexData.size());
		uvCoords.reserve(vertexData.size());
		tangents.reserve(vertexData.size());
		for (size_t vertIndex = 0; vertIndex < vertexData.size(); vertIndex++)
		{
			GFXVertex* const vertex = static_cast<GFXVertex*>(vertexData[vertIndex]);
			positions.emplace_back(vertex->Position);
			normals.emplace_back(vertex->Normal);
			uvCoords.emplace_back(vertex->UVData);
			tangents.emplace_back(vertex->Tangent);
		}

		mNumIndices = indices.size();
	}
}
