#pragma once

#include <functional>
#include <queue>
#include <unordered_map>
#include <vector>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/PrimitiveDefs.h>

namespace Diotima
{
	// DX12 Temp
	class IGFXDriverInterface;

	/////////////////
	class GFXMesh;
	class GFXMaterial;
	class GFXShaderPipeline;
	class GFXTexture2D;

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
		~Renderer();

		GFXShaderPipeline* mForwardShader;
		GFXShaderPipeline* mDepthPassShader;

	public:
		void Initialize();
		void Update();
		void ShutDown();

		void DX12Initialize();

	public:
		Int32 AddRenderItem(const RenderItemProtocol& itemProtocol);
		void RemoveRenderItem(const U32 itemIdx);

		Int32 AddLightItem(const LightItemProtocol& itemProtocol);
		inline RenderItemProtocol& GetItemProtocolByIdx(Int32 idx) { return mRenderList[idx]; }
		inline LightItemProtocol& GetLightProtocolByIdx(Int32 idx) { return mLightingList[idx]; }

		void SetCamera(const CameraItemProtocol& cameraItem) { camera = std::move(cameraItem); }

		void EnableVsync(bool bEnable);
		void ResizeCanvas(const Vector2D& newSize);
		
	private:
		Vector2D mCanvasSize;

		CameraItemProtocol camera;
		std::vector<RenderItemProtocol> mRenderList;
		std::vector<LightItemProtocol> mLightingList;

		std::queue<Int32> mFreeRenderListIndices;
		
		// DX12 Temp
	private:
		std::unique_ptr<IGFXDriverInterface> mDriverInterface;
	};
}
