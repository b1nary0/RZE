#include <Diotima/Graphics/Mesh.h>

#include <Assimp/Importer.hpp>
#include <Assimp/postprocess.h>
#include <Assimp/scene.h>

#include <Diotima/Driver/OpenGL.h>

#include <Utils/DebugUtils/Debug.h>

namespace Diotima
{
	GFXMesh::GFXMesh()
	{
		mVAO.Init();
		mVAO.Bind();

		mVertexVBO.Init();
		mVertexVBO.Bind();

		mNormalVBO.Init();
		mNormalVBO.Bind();

		mEBO.Init();
		mEBO.Bind();
	}

	GFXMesh::~GFXMesh()
	{
	}

	OpenGLVAO& GFXMesh::GetVAO()
	{
		return mVAO;
	}

	OpenGLVBO& GFXMesh::GetVertexVBO()
	{
		return mVertexVBO;
	}

	OpenGLEBO& GFXMesh::GetEBO()
	{
		return mEBO;
	}

	const std::vector<GFXVertex>& GFXMesh::GetVertexList() const
	{
		return mVertices;
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

		const GLsizeiptr verticesSize = mPositions.size() * sizeof(Vector3D);
		const GLsizeiptr normalsSize = mNormals.size() * sizeof(Vector3D);
		const GLsizeiptr uvDataSize = mUVCoords.size() * sizeof(Vector2D);
		const GLsizeiptr totalSize = verticesSize + normalsSize + uvDataSize;

		const GLsizeiptr normalsStart = verticesSize;
		const GLsizeiptr uvDataStart = normalsStart + normalsSize;

		const GLvoid* normalsStartPtr = reinterpret_cast<GLvoid*>(normalsStart);
		const GLvoid* uvDataStartPtr = reinterpret_cast<GLvoid*>(uvDataStart);

		mVAO.Bind();
		mVertexVBO.SetBufferData(nullptr, totalSize);
		mVertexVBO.SetBufferSubData(mPositions.data(), 0, verticesSize);
		mVertexVBO.SetBufferSubData(mNormals.data(), normalsStart, normalsSize);
		mVertexVBO.SetBufferSubData(mUVCoords.data(), uvDataStart, uvDataSize);

		mEBO.SetBufferData(mIndices.data(), sizeof(U32) * mIndices.size());

		// vertices
		OpenGLRHI::Get().EnableVertexAttributeArray(0);
		OpenGLRHI::Get().VertexAttribPointer(0, 3, EGLDataType::Float, EGLBooleanValue::False, sizeof(Vector3D), nullptr);

		// normals
		OpenGLRHI::Get().EnableVertexAttributeArray(1);
		OpenGLRHI::Get().VertexAttribPointer(1, 3, EGLDataType::Float, EGLBooleanValue::False, sizeof(Vector3D), normalsStartPtr);

		// tex coords
		OpenGLRHI::Get().EnableVertexAttributeArray(2);
		OpenGLRHI::Get().VertexAttribPointer(2, 2, EGLDataType::Float, EGLBooleanValue::False, sizeof(Vector2D), uvDataStartPtr);

		mVAO.Unbind();
	}

	void GFXMesh::AddIndex(U32 index)
	{
		mIndices.push_back(index);
	}
}
