#include <Diotima/Graphics/Mesh.h>

#include <Assimp/Importer.hpp>
#include <Assimp/postprocess.h>
#include <Assimp/scene.h>

#include <Diotima/Driver/OpenGL.h>
#include <Diotima/Graphics/Texture2D.h>

#include <Utils/DebugUtils/Debug.h>

namespace Diotima
{
	GFXMesh::GFXMesh()
	{
	}

	GFXMesh::~GFXMesh()
	{
	}

	const std::vector<U32>& GFXMesh::GetIndices() const
	{
		return mIndices;
	}

	void GFXMesh::AddVertex(const GFXVertex& vertex)
	{
		mVertices.push_back(std::move(vertex));
	}

	void GFXMesh::OnLoadFinished()
	{
		for (size_t vertIdx = 0; vertIdx < mVertices.size(); vertIdx++)
		{
			mPositions.push_back(mVertices[vertIdx].Position);
			mNormals.push_back(mVertices[vertIdx].Normal);
			mUVCoords.push_back(mVertices[vertIdx].UVData);
		}
	}

	void GFXMesh::AddIndex(U32 index)
	{
		mIndices.push_back(index);
	}

	void GFXMesh::AddTexture(GFXTexture2D* const texture)
	{
		if (texture->GetTextureType() == ETextureType::Diffuse)
		{
			mDiffuseTextures.emplace_back(texture);
		}
		else if (texture->GetTextureType() == ETextureType::Specular)
		{
			mSpecularTextures.emplace_back(texture);
		}
	}

}
