#pragma once

#include <functional>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <vector>

#include <Diotima/RenderCommands.h>
#include <Diotima/Graphics/Texture.h>

#include <Utils/PrimitiveDefs.h>
#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Platform/FilePath.h>

struct ID3D11InputLayout;
struct ID3D10Blob;
struct ID3D11VertexShader;
struct ID3D11PixelShader;

namespace Diotima
{
	// #DESIGN
	// 	class RenderResource
	// 	{
	// 	public:
	// 		RenderResource() = default;
	// 
	// 	private:
	// 		GPUResourceOfType* mResource;
	// 	};

	class DX11GFXDevice;

	class GFXPassGraph;
	class RenderTargetTexture;

	// #TODO
	// This is just here as a reminder to figure this out.
	// This is how the driver state needs to be set to draw an item.
	// Might be a good idea to investigate using the opt-in/out defaults system.
	// If something hasn't been overridden or excluded, it will use a default state.
	// Then there is no state leakage because everything just sets what it needs.
	struct GraphicsState
	{

	};

	// Essentially a direct copy starting at sizeof(GPUBufferData) into a constant buffer. 
	struct MaterialData
	{
		struct MaterialProperties
		{
			float Shininess;
		};

		Int32 mShaderID;
		TexturePack* mTexturePack;
		MaterialProperties mProperties;
	};

	struct MeshData
	{
		std::vector<float> Vertices;
		std::vector<U32> Indices;
		MaterialData Material;
	};

	// [newrenderer]
	// Not permanent
	struct CameraData
	{
		Vector3D Position;
		Matrix4x4 ProjectionMat;
		Matrix4x4 ViewMat;

		float FOV;
		float AspectRatio;
		float NearCull;
		float FarCull;
	};

	class Renderer
	{
	private:
		// Bundle of data that is necessary for each draw call.
		// #TODO
		// These will need to be wrapped or otherwise sorted properly by some higher-level design.
		// This is simply the data for any item with geometry. When and how this data is used is determined
		// elsewhere. The Int32 handles should be designed such that -1 means no data. 
		// For now, each item will need a Vertex and Index buffer, but should be easily extensible
		// to support other types of data construction or use.
		struct RenderObject
		{
			Int32 VertexBuffer = -1;
			Int32 IndexBuffer = -1;
			Int32 ConstantBuffer = -1;
			Int32 MaterialDataBuffer = -1;
			MaterialData Material;
			Matrix4x4 Transform;
		};

		// Constructors
	public:
		Renderer();
		~Renderer();

		// [newrenderer]
	public:
		void SetCameraData(
			const Vector3D& position,
			const Matrix4x4& projectionMat,
			const Matrix4x4& viewMat,
			float FOV,
			float aspectRatio,
			float nearCull,
			float farCull);

	private:
		void PrepareDrawState();
		void Draw();

	public:
		void Initialize();
		void Update();
		void Render();
		void ShutDown();

	public:
		
		// #TODO(Josh::Really really don't like this, fix later)
		void SetWindow(void* handle) { mWindowHandle = handle; }

		void EnableVsync(bool bEnable);

		const Vector2D& GetCanvasSize();
		void ResizeCanvas(const Vector2D& newSize);

		void SetViewportSize(const Vector2D& newSize);
		const Vector2D& GetViewportSize();

		// #TODO
		// private later
		Int32 CreateVertexBuffer(void* data, size_t size, U32 count);
		Int32 CreateIndexBuffer(void* data, size_t size, U32 count);
		Int32 CreateTextureBuffer2D(void* data, U32 width, U32 height);
		Int32 CreatePixelShader(const FilePath& filePath);

		// [newrenderer]
		// This is just to fix allocating resources every frame. Need to track the RenderObject
		// created, but ideally this should **NOT** just be U32 based on the index of the item.
		U32 CreateRenderObject(
			const MeshData& meshData, 
			const std::vector<TextureData>& textureData, 
			const Matrix4x4& transform);
		
		void UpdateRenderObject(U32 renderObjectHandle, const Matrix4x4& newTransform);

	private:
		void InitializeRenderObject(
			RenderObject& renderObject,
			const MeshData& meshData,
			const std::vector<TextureData>& textureData,
			const Matrix4x4& transform);

		void ProcessCommands();

		// #TODO
		// Turn this into a command structure. Something like:
		// UpdateRenderObject<UpdateTransformCommand>(renderObject);
		// Where UpdateTransformCommand:
		// 
		// class UpdateTransformCommand : public RenderCommand
		// {
		// public:
		//		UpdateTransformCommand(const RenderObject& renderObject, const Matrix4x4& transform);
		// 		virtual void Execute();
		// private:
		//		RenderObject& renderObject;
		// 		Matrix4x4 transform;
		// };

		void UpdateRenderObjectTransform_GPU(const RenderObject& renderObject);

	private:
		// #TODO
		// Eventually needs to sort on key.
		std::vector<RenderObject> mRenderObjects;
		std::queue<U32> mFreeRenderObjectIndices;

		Vector2D mCanvasSize;
		RenderTargetTexture* mRenderTarget;

	private:
		void* mWindowHandle;
		std::unique_ptr<DX11GFXDevice> mDevice;
		Vector2D mViewportDimensions;

		CameraData mCameraData;

		//
		// Buckets
		//
		// std::vector<DrawBucket> mBuckets;
		//
		// void Draw()
		// {
		//		for (const DrawBucket& bucket : mBuckets)
		//		{
		//			Draw bucket.RenderObject[i] with bucket.DrawState
		//		}
		// }
		//
		// BucketProxy* bucketProxy = Renderer::StartBucket();
		// bucketProxy->SetDrawState(someState);
		// bucketProxy->AddRenderObjectThatPassedSomeCullingOperation(someObject);
		// Renderer::SubmitBucket(bucketProxy); // We're done here, bucketProxy invalid now.
	};
}
