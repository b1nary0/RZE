#pragma once

#include <functional>
#include <queue>
#include <unordered_map>
#include <vector>

#include <Diotima/Driver/OpenGL.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector2D.h>

namespace Diotima
{
	class GFXMesh;
	class GFXMaterial;
	class GFXShaderPipeline;
	class GFXTexture2D;

	class RenderBatch;
	class RenderTarget;
	
	class Renderer
	{
	public:
		struct RenderItemProtocol
		{
			RenderItemProtocol();

			std::vector<GFXTexture2D*>		Textures;
			Matrix4x4						ModelMat;
			RenderBatch*					BatchData{ nullptr };

			bool bIsValid{ false };

			void Invalidate();

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
