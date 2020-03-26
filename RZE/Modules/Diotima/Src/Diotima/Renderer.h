#pragma once

#include <functional>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <vector>

#include <Diotima/RenderCommands.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/PrimitiveDefs.h>

// #TODO(Josh::Temp)
#define MAX_LIGHTS 128

struct ID3D11InputLayout;

namespace Diotima
{
	class DX11GFXDevice;

	class GFXPassGraph;

	enum class EBufferType
	{
		BufferType_Vertex
	};

	class Renderer
	{
		// #TODO(Temp for refactor, for access to RenderItemDrawCall)
		friend class ForwardPass;
		friend class DepthPass;
		friend class ImGUIPass;

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
		};

		struct RenderItemProtocol
		{
			// #TODO(Josh::This needs to be done better. Works for now, but will need resolving when stuff matures)
			std::vector<RenderItemMeshData> MeshData;
			Matrix4x4						ModelMatrix;
			U32								ConstantBuffer;

			bool bIsValid{ false };
			void Invalidate();
		};

		enum ELightType : U32
		{
			Directional = 0,
			Point,
			Count
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

	private:
		// #TODO(Josh::Temp idea: RenderItemProtocol describes the data to render in it's highest level, 
		//             while RenderItemDrawCall contains all the buffer indirections to actually draw the item.
		//             Will iterate on this idea.)
		struct RenderItemDrawCall
		{
			U32 VertexBuffer;
			U32 IndexBuffer;
			U32 ConstantBuffer;
			U32 TextureSlot0;
			U32 TextureSlot1;
			U32 TextureSlot2;
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
		void RemoveLightItem(const U32 itemIdx);

		inline RenderItemProtocol& GetItemProtocolByIdx(Int32 idx) { return mRenderItems[idx]; }
		inline LightItemProtocol& GetLightProtocolByIdx(Int32 idx) { return mLightingList[idx]; }

		// #TODO(Josh::Really really don't like this, fix later)
		void SetWindow(void* handle) { mWindowHandle = handle; }
		void SetCamera(const CameraItemProtocol& cameraItem) { camera = std::move(cameraItem); }

		void EnableVsync(bool bEnable);
		void SetMSAASampleCount(U32 sampleCount);

		const Vector2D& GetCanvasSize();
		void ResizeCanvas(const Vector2D& newSize);

	public:
		U32 QueueCreateVertexBufferCommand(void* data, size_t size, U32 count);
		U32 QueueCreateIndexBufferCommand(void* data, size_t size, U32 count);
		U32 QueueCreateTextureCommand(ECreateTextureBufferType bufferType, void* data, U32 width, U32 height);

		void QueueUpdateRenderItem(U32 itemID, const Matrix4x4& worldMtx);

		void ProcessCommands();

	private:
		U32 CreateVertexBuffer(void* data, size_t size, U32 count);
		U32 CreateIndexBuffer(void* data, size_t size, U32 count);
		U32 CreateTextureBuffer2D(void* data, U32 width, U32 height);

	private:
		const std::vector<RenderItemDrawCall>& GetDrawCalls();
		const std::vector<LightItemProtocol>& GetLights();
		const U32* GetLightCounts();
		const CameraItemProtocol& GetCamera();

		void PrepareDrawCalls();

	private:
		CameraItemProtocol camera;
		Vector2D mCanvasSize;

		U32 mLightCounts[ELightType::Count] { 0 };

		std::vector<LightItemProtocol> mLightingList;
		std::vector<RenderItemProtocol> mRenderItems;
		std::vector<RenderItemDrawCall> mPerFrameDrawCalls;

		std::queue<Int32> mFreeRenderListIndices;
		
		// #TODO(Josh::Need this duplicated here and in the Device because it needs to be set prior to initialization)
		U32 mMSAASampleCount;

	private:
		std::vector<CreateBufferRenderCommand> mVertexBufferCommandQueue;
		std::vector<CreateBufferRenderCommand> mIndexBufferCommandQueue;
		std::vector<CreateTextureBufferRenderCommand> mTextureBufferCommandQueue;
		std::vector<UpdateRenderItemWorldMatrixCommand> mUpdateRenderItemWorldMatrixCommandQueue;

		// DX12 Temp
	private:
		U32 mViewProjBuf;
		U32 mLightBuf;
		U32 mCameraDataBuf;

		std::unique_ptr<GFXPassGraph> mPassGraph;
		
		std::unique_ptr<DX11GFXDevice> mDevice;

		void* mWindowHandle;

		ID3D11InputLayout* mVertexLayout;

	private:
		std::mutex mVertexBufferCommandMutex;
		std::mutex mIndexBufferCommandMutex;
		std::mutex mTextureBufferCommandMutex;
		std::mutex mUpdateRenderItemWorldMatrixCommandMutex;
	};
}
