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
		void RenderToTexture();

	private:
		Vector2D mCanvasSize;

		CameraItemProtocol camera;
		std::vector<RenderItemProtocol> mRenderList;
		std::vector<LightItemProtocol> mLightingList;

		std::queue<Int32> mFreeRenderListIndices;

		RenderTarget* mRenderTarget { nullptr };
	};
}
