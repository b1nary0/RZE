#include <Diotima/RenderBatch.h>

#include <Diotima/Graphics/Mesh.h>

#include <Utils/PrimitiveDefs.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>

namespace Diotima
{
	RenderBatch::RenderBatch()
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

	void RenderBatch::Allocate(const std::vector<GFXMesh*>& meshes)
	{
		std::vector<Vector3D> allPositions;
		std::vector<Vector3D> allNormals;
		std::vector<Vector2D> allUVData;
		std::vector<U32> allIndices;

		for (auto& mesh : meshes)
		{
			allPositions.insert(allPositions.end(), mesh->GetPositions().begin(), mesh->GetPositions().end());
			allNormals.insert(allNormals.end(), mesh->GetNormals().begin(), mesh->GetNormals().end());
			allUVData.insert(allUVData.end(), mesh->GetUVCoords().begin(), mesh->GetUVCoords().end());
			allIndices.insert(allIndices.end(), mesh->GetIndices().begin(), mesh->GetIndices().end());
		}

		const GLsizeiptr verticesSize = allPositions.size() * sizeof(Vector3D);
		const GLsizeiptr normalsSize = allNormals.size() * sizeof(Vector3D);
		const GLsizeiptr uvDataSize = allUVData.size() * sizeof(Vector2D);
		const GLsizeiptr totalSize = verticesSize + normalsSize + uvDataSize;

		const GLsizeiptr normalsStart = verticesSize;
		const GLsizeiptr uvDataStart = normalsStart + normalsSize;

		const GLvoid* normalsStartPtr = reinterpret_cast<GLvoid*>(normalsStart);
		const GLvoid* uvDataStartPtr = reinterpret_cast<GLvoid*>(uvDataStart);

		mVAO.Bind();
		mVertexVBO.SetBufferData(nullptr, totalSize);
		mVertexVBO.SetBufferSubData(allPositions.data(), 0, verticesSize);
		mVertexVBO.SetBufferSubData(allNormals.data(), normalsStart, normalsSize);
		mVertexVBO.SetBufferSubData(allUVData.data(), uvDataStart, uvDataSize);

		mEBO.SetBufferData(allIndices.data(), sizeof(U32) * allIndices.size());

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

		mNumIndices = allIndices.size();
	}

}