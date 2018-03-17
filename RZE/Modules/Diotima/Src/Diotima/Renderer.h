#pragma once

#include <functional>
#include <queue>
#include <unordered_map>
#include <vector>

#include <Diotima/SceneCamera.h>
#include <Diotima/Driver/OpenGL.h>
#include <Diotima/Driver/GLRenderTarget.h>
#include <Diotima/Graphics/Material.h>

#include <Utils/Interfaces/SubSystem.h>
#include <Utils/Math/Vector2D.h>

namespace Diotima
{
	class GFXMesh;
	class GFXMaterial;
	class GFXShaderPipeline;
	class GFXTexture2D;
	
	class Renderer : public ISubSystem
	{
	public:
		typedef struct RenderItemProtocol
		{
			RenderItemProtocol();

			GFXShaderPipeline*					Shader { nullptr };
			GFXMaterial						Material;
			std::vector<GFXTexture2D*>		Textures;
			Matrix4x4						ModelMat;
			Matrix4x4						ProjectionMat;
			Matrix4x4						ViewMat;
			std::vector<GFXMesh*>*			MeshData { nullptr };

			bool bIsValid{ false };

			void Invalidate()
			{
				MeshData = nullptr;
				Textures.clear();

				bIsValid = false;
			}

		} RenderItemProtocol;

		typedef struct LightItemProtocol
		{
			Vector3D	Position;
			Vector3D	Color;
			
			float		Strength;
		} LightItemProtocol;

		typedef struct CameraItemProtocol
		{
			Vector3D Position;
			Matrix4x4 ProjectionMat;
			Matrix4x4 ViewMat;

			float FOV;
			float AspectRatio;
			float NearCull;
			float FarCull;
		} CameraItemProtocol;

		// Constructors
	public:
		// #TODO(Josh) Remove the parameters here and work in a better event driven system?
		Renderer();

		// ISubSystem interface
	public:
		virtual void Initialize();
		virtual void Update();
		virtual void ShutDown();

	public:
		Int32 AddRenderItem(const RenderItemProtocol& itemProtocol);
		void RemoveRenderItem(const U32 itemIdx);

		inline RenderItemProtocol& GetItemProtocolByIdx(Int32 idx) { return mRenderList[idx]; }
		Int32 AddLightItem(const LightItemProtocol& itemProtocol);
		inline LightItemProtocol& GetLightProtocolByIdx(Int32 idx) { return mLightingList[idx]; }

		void SetCamera(const CameraItemProtocol& cameraItem) { camera = std::move(cameraItem); }

		void ResizeCanvas(const Vector2D& newSize);

		void ClearLists();
		
	private:
		void RenderSingleItem(RenderItemProtocol& itemProtocol);

		void RenderToTexture_Test(RenderItemProtocol& itemProtocol);

	private:
		GLRenderTargetTexture mRenderTargetTexture;

		CameraItemProtocol camera;
		std::vector<RenderItemProtocol> mRenderList;
		std::vector<LightItemProtocol> mLightingList;

		std::queue<Int32> mFreeRenderListIndices;
	};
}
