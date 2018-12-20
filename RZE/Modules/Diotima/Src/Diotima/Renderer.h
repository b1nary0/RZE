#pragma once

#include <functional>
#include <queue>
#include <unordered_map>
#include <vector>

#include <Diotima/Driver/OpenGL/OpenGL.h>

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

	class GLRenderTargetDepthTexture;
	
	class Renderer
	{
	public:
		struct RenderItemProtocol
		{
			RenderItemProtocol();

			std::vector<GFXMesh*>			MeshData;
			Matrix4x4						ModelMatrix;

			bool bIsValid{ false };

			void Invalidate();

		};

		struct LightItemProtocol
		{
			Vector3D	Position;
			Vector3D	Color;
			Matrix4x4	LightSpaceMatrix;

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

		GFXShaderPipeline* mForwardShader;
		GFXShaderPipeline* mDepthPassShader;

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
		
		// The below will generally be replaced by a proper implementation
	private:
		void DepthPass();
		void ForwardPass();

		void RenderScene_Forward();
		void RenderScene_Depth();

		void RenderSingleItem_Forward(RenderItemProtocol& renderItem);
		void RenderSingleItem_Depth(RenderItemProtocol& renderItem);

		void SetCurrentRenderTarget(RenderTarget* renderTarget);
		RenderTarget* GetCurrentRenderTarget() { return mCurrentRTT; }

	private:
		void DrawMesh(GFXMesh* mesh);

		void BlitToWindow();
		void BlitToTarget(const RenderTarget& target);

		void Submit();

	private:
		Vector2D mCanvasSize;

		CameraItemProtocol camera;
		std::vector<RenderItemProtocol> mRenderList;
		std::vector<LightItemProtocol> mLightingList;

		std::queue<Int32> mFreeRenderListIndices;
		
		RenderTarget* mCustomRTT { nullptr };
		RenderTarget* mCurrentRTT { nullptr };
		RenderTarget* mFinalRTT { nullptr };

		GLRenderTargetDepthTexture* mDepthTexture { nullptr };
	};
}
