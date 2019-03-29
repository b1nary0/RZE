#pragma once

#include <functional>
#include <queue>
#include <unordered_map>
#include <vector>

// #TODO(Josh::Really don't like this, change later)
#include <Diotima/Driver/DX12/DX12AllocationData.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/PrimitiveDefs.h>

namespace Diotima
{
	// DX12 Temp
	class DX12GFXDevice;

	enum class EBufferType
	{
		BufferType_Vertex
	};

	class Renderer
	{
	public:
		enum class ETextureType
		{
			Diffuse,
			Specular,
			Normal
		};

		struct RenderItemTextureDesc
		{
			RenderItemTextureDesc(U32 textureBuffer, ETextureType textureType)
				: TextureBuffer(textureBuffer)
				, TextureType(textureType) {}

			U32 TextureBuffer;
			ETextureType TextureType;
		};

		struct RenderItemMaterialDesc
		{
			float Shininess;
		};

		struct RenderItemMeshData
		{
			U32 VertexBuffer;
			U32 IndexBuffer;
			std::vector<RenderItemTextureDesc> TextureDescs;
			RenderItemMaterialDesc Material;
			// #TODO(Josh::These members should be temporary. Need to find a better way to relate these to buffers)
			CBAllocationData MaterialBufferAllocData;
		};

		struct RenderItemProtocol
		{
			// #TODO(Josh::This needs to be done better. Works for now, but will need resolving when stuff matures)
			std::vector<RenderItemMeshData> MeshData;
			Matrix4x4						ModelMatrix;

			bool bIsValid{ false };
			void Invalidate();
		};

		enum ELightType : U32
		{
			Directional = 0,
			Point
		};

		struct LightItemProtocol
		{
			Vector4D	Position;
			Vector4D	Color;
			Matrix4x4	LightSpaceMatrix;
			float		Strength;
			ELightType	LightType;
			Vector2D	Padding;
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
		void SetMSAASampleCount(U32 sampleCount);

		void ResizeCanvas(const Vector2D& newSize);

		// #TODO(Josh::Stand-ins for command infrastructure until DX12 rendering stabilized)
		U32 CreateVertexBuffer(void* data, U32 numElements);
		U32 CreateIndexBuffer(void* data, U32 numElements);
		U32 CreateTextureBuffer2D(void* data, U32 width, U32 height);

	private:
		void DX12Initialize();
		
		void PrepareLights();
		void PrepareMaterials();

	private:
		Vector2D mCanvasSize;

		CameraItemProtocol camera;
		std::vector<RenderItemProtocol> mRenderList;
		std::vector<LightItemProtocol> mLightingList;

		std::queue<Int32> mFreeRenderListIndices;
		
		// #TODO(Josh::Need this duplicated here and in the Device because it needs to be set prior to initialization)
		U32 mMSAASampleCount;

		// DX12 Temp
	private:
		U32 mMVPConstantBuffer;
		U32 mLightConstantBuffer;
		U32 mMaterialBuffer; // Per mesh data
		U32 mPerFramePixelShaderConstants; // Per frame data

		std::unique_ptr<DX12GFXDevice> mDevice;

		void* mWindowHandle;
	};
}
