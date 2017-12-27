#pragma once

#include <unordered_map>
#include <vector>

#include <Diotima/Driver/OpenGL.h>
#include <Diotima/Graphics/Texture2D.h>

#include <Utils/Interfaces/Resource.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>

struct aiMesh;
struct aiNode;
struct aiScene;

namespace Diotima
{
	class GFXTexture2D;

	struct GFXVertex
	{
		Vector3D Position;
		Vector3D Normal;
		Vector2D UVData;
	};

	class GFXMesh
	{
		friend class MeshResource;

		typedef std::unordered_map<U32, std::vector<GFXTexture2D*>> TextureMap;

	public:
		GFXMesh();
		~GFXMesh();

		OpenGLVAO& GetVAO();
		OpenGLEBO& GetEBO();
		OpenGLVBO& GetVertexVBO();

		const std::vector<GFXVertex>& GetVertexList() const;
		const std::vector<U32>& GetIndices() const;
		const std::vector<GFXTexture2D*>& GetDiffuseTextures() { return mDiffuseTextures; }
		const std::vector<GFXTexture2D*>& GetSpecularTextures() { return mSpecularTextures; }

		void AddVertex(const GFXVertex& vertex);
		void AddIndex(U32 index);
		void AddTexture(GFXTexture2D* const texture);

		void OnLoadFinished();

	private:
		OpenGLVAO mVAO;
		OpenGLVBO mVertexVBO;
		OpenGLVBO mNormalVBO;
		OpenGLEBO mEBO;

		std::vector<GFXTexture2D*> mDiffuseTextures;
		std::vector<GFXTexture2D*> mSpecularTextures;

		std::vector<GFXVertex> mVertices;
		std::vector<Vector3D> mPositions;
		std::vector<Vector3D> mNormals;
		std::vector<Vector2D> mUVCoords;
		std::vector<U32> mIndices;

		U32 mDiffuseTextureCount;
		U32 mSpecularTextureCount;
	};
}
