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
	class Renderer
	{
		// Constructors
	public:
		Renderer();
		~Renderer();
		
	public:
		static void Initialize(void* mWindowHandle);
		static void ShutDown();
		
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
		static std::unique_ptr<DX11GFXDevice> m_device;
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
