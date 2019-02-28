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
	class DX12GFXDriverInterface;

	/////////////////
	class GFXMesh;
	class GFXMaterial;
	class GFXShaderPipeline;
	class GFXTexture2D;

	enum class EBufferType
	{
		BufferType_Vertex
	};

	class Renderer
	{
	public:
		struct RenderItemProtocol
		{
			RenderItemProtocol();

			// #TODO(Josh::This needs to be done better. Works for now, but will need resolving when stuff matures)
			std::vector<U32>								VertexBuffers;
			std::vector<U32>								IndexBuffers;

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
		void Render();
		void ShutDown();

	public:
		Int32 AddRenderItem(const RenderItemProtocol& itemProtocol);
		void RemoveRenderItem(const U32 itemIdx);

		Int32 AddLightItem(const LightItemProtocol& itemProtocol);
		inline RenderItemProtocol& GetItemProtocolByIdx(Int32 idx) { return mRenderList[idx]; }
		inline LightItemProtocol& GetLightProtocolByIdx(Int32 idx) { return mLightingList[idx]; }

		// #TODO(Josh::Really really don't like this, fix later)
		void SetWindow(void* handle) { mWindowHandle = handle; }
		void SetCamera(const CameraItemProtocol& cameraItem) { camera = std::move(cameraItem); }

		void EnableVsync(bool bEnable);
		void ResizeCanvas(const Vector2D& newSize);

		// #TODO(Josh::Stand-ins for command infrastructure until DX12 rendering stabilized)
		U32 CreateVertexBuffer(void* data, U32 numElements);
		U32 CreateIndexBuffer(void* data, U32 numElements);

	private:
		void DX12Initialize();
		
	private:
		Vector2D mCanvasSize;

		CameraItemProtocol camera;
		std::vector<RenderItemProtocol> mRenderList;
		std::vector<LightItemProtocol> mLightingList;

		std::queue<Int32> mFreeRenderListIndices;
		
		// DX12 Temp
	private:
		std::unique_ptr<DX12GFXDriverInterface> mDriverInterface;

		void* mWindowHandle;
	};
}
