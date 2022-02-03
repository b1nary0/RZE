#pragma once

#include <Rendering/Driver/GFXBuffer.h>

#include <memory>

class Vector2D;
class Vector4D;

namespace Rendering
{
	class DX11Device;
	class IVertexBuffer;

	class VertexBufferHandle
	{
		friend class Renderer;

	public:
		VertexBufferHandle() = default;
		~VertexBufferHandle() = default;

	private:
		VertexBufferHandle(const std::shared_ptr<IGPUBuffer>& buffer)
		{
			m_buffer = buffer;
		}

		std::shared_ptr<IGPUBuffer> m_buffer;
	};

	class IndexBufferHandle
	{
		friend class Renderer;

	public:
		IndexBufferHandle() = default;
		~IndexBufferHandle() = default;

	private:
		IndexBufferHandle(const std::shared_ptr<IGPUBuffer>& buffer)
		{
			m_buffer = buffer;
		}

		std::shared_ptr<IGPUBuffer> m_buffer;
	};

	class Renderer
	{
	private:
		struct RenderState
		{
			// This will get filled out per Begin/End call
		};

	public:
		struct ViewportParams
		{
			float Width;
			float Height;
			float MinDepth;
			float MaxDepth;
			float TopLeftX;
			float TopLeftY;
		};

		// Constructors
	public:
		Renderer();
		~Renderer();
		
	public:
		static void Initialize(void* mWindowHandle);
		static void Shutdown();

		// API for an entire frame (global state, etc)
		static void BeginFrame();
		static void EndFrame();

		// API for a render pass (series of draw calls)
		static void Begin();
		static void End();

		static void HandleWindowResize(const Vector2D& newSize);

	public:
		static VertexBufferHandle CreateVertexBuffer(void* data, size_t dataTypeSize, size_t count);
		static IndexBufferHandle CreateIndexBuffer(void* data, size_t dataTypeSize, size_t count);

	public:
		static void SetClearColour(const Vector4D& colour);

		static void SetViewport(const ViewportParams& viewportParams);

	private:
		//void ProcessCommands();

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
		
	private:
		static std::unique_ptr<DX11Device> m_device;
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
