#pragma once

#include <Rendering/RenderCommands.h>
#include <Rendering/Graphics/Texture.h>

#include <Utils/PrimitiveDefs.h>
#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Platform/FilePath.h>

#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <vector>

namespace Rendering
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

	class IGFXVertexBuffer;
	class IGFXIndexBuffer;
	class IGFXConstantBuffer;

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
			float Opacity;
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

	struct RenderObject
	{
		bool bEnabled = false;
		IGFXVertexBuffer* VertexBuffer = nullptr;
		IGFXIndexBuffer* IndexBuffer = nullptr;
		IGFXConstantBuffer* ConstantBuffer = nullptr;
		IGFXConstantBuffer* MaterialDataBuffer = nullptr;
		MaterialData Material;
		Matrix4x4 Transform;
	};

	class Renderer
	{
		// Constructors
	public:
		Renderer();
		~Renderer();

		// [newrenderer]
	public:
		void SetCameraData(const CameraData& cameraData);

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

		void SetRenderTarget(RenderTargetTexture* renderTarget);

		// #TODO
		// private later
		IGFXVertexBuffer* CreateVertexBuffer(void* data, size_t size, U32 count);
		IGFXIndexBuffer* CreateIndexBuffer(void* data, size_t size, U32 count);
		Int32 CreateTextureBuffer2D(void* data, U32 width, U32 height);
		Int32 CreatePixelShader(const FilePath& filePath);

		// [newrenderer]
		// This is just to fix allocating resources every frame. Need to track the RenderObject
		// created, but ideally this should **NOT** just be U32 based on the index of the item.
		RenderObject* CreateAndInitializeRenderObject(
			const MeshData& meshData, 
			const std::vector<TextureData>& textureData, 
			const Matrix4x4& transform);

		void DestroyRenderObject(RenderObject* renderObject);
		
		void UpdateRenderObject(RenderObject* renderObject, const Matrix4x4& newTransform);

		void AddRenderObject(RenderObject* renderObject);

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
		std::vector<RenderObject*> mRenderObjects;

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
		// ^^^^^ This looks like it should be some reference-type structure/architecture
	};
}
