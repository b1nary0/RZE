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

		mVAO.Unbind();
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

		const GLsizeiptr verticesSize = positions.size() * sizeof(Vector3D);
		const GLsizeiptr normalsSize = normals.size() * sizeof(Vector3D);
		const GLsizeiptr tangentSize = tangents.size() * sizeof(Vector3D);
		const GLsizeiptr uvDataSize = uvCoords.size() * sizeof(Vector2D);

		const GLsizeiptr totalSize = verticesSize + normalsSize + tangentSize + uvDataSize;

		const GLsizeiptr normalsStart = verticesSize;
		const GLsizeiptr tangentStart = normalsStart + normalsSize;
		const GLsizeiptr uvDataStart = tangentStart + tangentSize;

		const GLvoid* normalsStartPtr = reinterpret_cast<GLvoid*>(normalsStart);
		const GLvoid* tangentStartPtr = reinterpret_cast<GLvoid*>(tangentStart);
		const GLvoid* uvDataStartPtr = reinterpret_cast<GLvoid*>(uvDataStart);

		mVAO.Bind();
		mVertexVBO.SetBufferData(nullptr, totalSize);
		mVertexVBO.SetBufferSubData(positions.data(), 0, verticesSize);
		mVertexVBO.SetBufferSubData(normals.data(), normalsStart, normalsSize);
		mVertexVBO.SetBufferSubData(tangents.data(), tangentStart, tangentSize);
		mVertexVBO.SetBufferSubData(uvCoords.data(), uvDataStart, uvDataSize);

		mEBO.SetBufferData(indices.data(), sizeof(U32) * indices.size());

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

		mVAO.Unbind();
		mNumIndices = indices.size();
	}
}
