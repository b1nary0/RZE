#pragma once

#include <functional>
#include <queue>
#include <unordered_map>
#include <vector>

#include <Diotima/SceneCamera.h>
#include <Diotima/Driver/OpenGL.h>
#include <Diotima/Driver/GLRenderTarget.h>
#include <Diotima/Graphics/Material.h>

#include <Utils/Math/Vector2D.h>




// #TODO(Josh::Maybe start by thinking about just calling draw on a mesh, with all other things handled before
//				i.e: Cull, Sort (by Material, Depth, etc), Merge?, build Command Buffer, submit)
//				
//				Build on this idea.


#if 1
#include <Diotima/Graphics/Mesh.h>
namespace Diotima
{
	class GFXMesh;

	class RenderBatch
	{
	public:
		RenderBatch() 
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

		void Allocate(const std::vector<GFXMesh*>& meshes)
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

	public:
		OpenGLVAO mVAO;
		OpenGLVBO mVertexVBO;
		OpenGLVBO mNormalVBO;
		OpenGLEBO mEBO;

		U32 mNumIndices;
	};
}
#endif


namespace Diotima
{
	class GFXMesh;
	class GFXMaterial;
	class RenderTargetTexture;
	class GFXShaderPipeline;
	class GFXTexture2D;
	
	class Renderer
	{
	public:
		struct RenderItemProtocol
		{
			RenderItemProtocol();

			GFXShaderPipeline*					Shader{ nullptr };
			GFXMaterial						Material;
			std::vector<GFXTexture2D*>		Textures;
			Matrix4x4						ModelMat;
			std::vector<GFXMesh*>*			MeshData{ nullptr };
			RenderBatch*					BatchData{ nullptr };

			bool bIsValid{ false };

			void Invalidate()
			{
				MeshData = nullptr;
				Textures.clear();

				bIsValid = false;
			}

		};

		struct LightItemProtocol
		{
			Vector3D	Position;
			Vector3D	Color;

			float		Strength;
		};

		struct CameraItemProtocol
		{
			Vector3D Position;
			Matrix4x4 ProjectionMat;
			Matrix4x4 ViewMat;

			float FOV;
			float AspectRatio;
			float NearCull;
			float FarCull;
		};

		// Constructors
	public:
		Renderer();

		GFXShaderPipeline* mShaderPipeline;

		// ISubSystem interface
	public:
		void Initialize();
		void Update();
		void ShutDown();

	public:
		Int32 AddRenderItem(const RenderItemProtocol& itemProtocol);
		void RemoveRenderItem(const U32 itemIdx);

		Int32 AddLightItem(const LightItemProtocol& itemProtocol);
		inline RenderItemProtocol& GetItemProtocolByIdx(Int32 idx) { return mRenderList[idx]; }
		inline LightItemProtocol& GetLightProtocolByIdx(Int32 idx) { return mLightingList[idx]; }

		void SetRenderTarget(RenderTarget* renderTarget);
		void SetCamera(const CameraItemProtocol& cameraItem) { camera = std::move(cameraItem); }

		void EnableVsync(bool bEnable);
		void ResizeCanvas(const Vector2D& newSize);
		
	private:
		void RenderSingleItem(RenderItemProtocol& itemProtocol);
		void BlitToWindow();

	private:
		Vector2D mCanvasSize;

		CameraItemProtocol camera;
		std::vector<RenderItemProtocol> mRenderList;
		std::vector<LightItemProtocol> mLightingList;

		std::queue<Int32> mFreeRenderListIndices;

		RenderTarget* mRenderTarget { nullptr };
	};
}
