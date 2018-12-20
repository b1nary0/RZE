#pragma once

#include <unordered_map>
#include <vector>

#include <Diotima/Driver/OpenGL/OpenGL.h>
#include <Diotima/Graphics/Texture2D.h>

#include <Utils/Interfaces/Resource.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>

struct aiMesh;
struct aiNode;
struct aiScene;

namespace Diotima
{
	class GFXMaterial;
	class GFXTexture2D;

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

		OpenGLVAO mVAO;
		OpenGLVBO mVertexVBO;
		OpenGLEBO mEBO;

		const std::vector<U32>& GetIndices() const;
		const GFXMaterial& GetMaterial() { return *mMaterial; }
		const std::vector<Vector3D>& GetPositions() const { return mPositions; }
		const std::vector<Vector3D>& GetNormals() const { return mNormals; }
		const std::vector<Vector2D>& GetUVCoords() const { return mUVCoords; }

		void AddVertex(const GFXVertex& vertex);
		void AddIndex(U32 index);
		void SetMaterial(GFXMaterial* material);

		void OnLoadFinished();

	private:
		GFXMaterial* mMaterial;

		std::vector<GFXVertex> mVertices;

		std::vector<Vector3D> mPositions;
		std::vector<Vector3D> mNormals;
		std::vector<Vector3D> mTangents;
		std::vector<Vector2D> mUVCoords;

		std::vector<U32> mIndices;
	};
}
