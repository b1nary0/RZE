#include <Diotima/Graphics/GFXMesh.h>

#include <Assimp/Importer.hpp>
#include <Assimp/postprocess.h>
#include <Assimp/scene.h>

#include <Diotima/Driver/OpenGL/OpenGL.h>
#include <Diotima/Graphics/GFXTexture2D.h>

#include <Utils/DebugUtils/Debug.h>

namespace Diotima
{
	GFXMesh::GFXMesh()
	{
		// #TODO(Josh::Move this into an Allocate() function or something - shoud be user/system initiated)
 		mVAO.Init();
 		mVAO.Bind();

		mVertexVBO.Init();
		mVertexVBO.Bind();

		mEBO.Init();
		mEBO.Bind();
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
			mTangents.push_back(mVertices[vertIdx].Tangent);
		}

		const GLsizeiptr verticesSize = mPositions.size() * sizeof(Vector3D);
		const GLsizeiptr normalsSize = mNormals.size() * sizeof(Vector3D);
		const GLsizeiptr tangentSize = mTangents.size() * sizeof(Vector3D);
		const GLsizeiptr uvDataSize = mUVCoords.size() * sizeof(Vector2D);

		const GLsizeiptr totalSize = verticesSize + normalsSize + tangentSize + uvDataSize;

		const GLsizeiptr normalsStart = verticesSize;
		const GLsizeiptr tangentStart = normalsStart + normalsSize;
		const GLsizeiptr uvDataStart = tangentStart + tangentSize;

		const GLvoid* normalsStartPtr = reinterpret_cast<GLvoid*>(normalsStart);
		const GLvoid* tangentStartPtr = reinterpret_cast<GLvoid*>(tangentStart);
		const GLvoid* uvDataStartPtr = reinterpret_cast<GLvoid*>(uvDataStart);

		mVAO.Bind();
		mVertexVBO.SetBufferData(nullptr, totalSize);
		mVertexVBO.SetBufferSubData(mPositions.data(), 0, verticesSize);
		mVertexVBO.SetBufferSubData(mNormals.data(), normalsStart, normalsSize);
		mVertexVBO.SetBufferSubData(mTangents.data(), tangentStart, tangentSize);
		mVertexVBO.SetBufferSubData(mUVCoords.data(), uvDataStart, uvDataSize);

		mEBO.SetBufferData(mIndices.data(), sizeof(U32) * mIndices.size());

		// vertices
		OpenGLRHI::Get().EnableVertexAttributeArray(0);
		OpenGLRHI::Get().VertexAttribPointer(0, 3, EGLDataType::Float, EGLBooleanValue::False, sizeof(Vector3D), nullptr);

		// normals
		OpenGLRHI::Get().EnableVertexAttributeArray(1);
		OpenGLRHI::Get().VertexAttribPointer(1, 3, EGLDataType::Float, EGLBooleanValue::False, sizeof(Vector3D), normalsStartPtr);

		// tangents
		OpenGLRHI::Get().EnableVertexAttributeArray(2);
		OpenGLRHI::Get().VertexAttribPointer(2, 3, EGLDataType::Float, EGLBooleanValue::False, sizeof(Vector3D), tangentStartPtr);

		// tex coords
		OpenGLRHI::Get().EnableVertexAttributeArray(3);
		OpenGLRHI::Get().VertexAttribPointer(3, 2, EGLDataType::Float, EGLBooleanValue::False, sizeof(Vector2D), uvDataStartPtr);
	}

	void GFXMesh::AddIndex(U32 index)
	{
		mIndices.push_back(index);
	}

	void GFXMesh::SetMaterial(GFXMaterial* material)
	{
		AssertNotNull(material);
		mMaterial = material;
	}
}
