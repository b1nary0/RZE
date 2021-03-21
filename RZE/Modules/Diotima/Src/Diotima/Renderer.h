#pragma once

#include <RZE_Config.h>

#if WITH_NEW_RENDERER
#include <functional>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <vector>

#include <Diotima/RenderCommands.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/PrimitiveDefs.h>

struct ID3D11InputLayout;

namespace Diotima
{
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

	// Essentially a direct copy into a constant buffer. All this data is consumed by the GPU
	// for lighting etc
	struct MaterialData
	{
		Int32 TextureBuffer = -1;
		Int32 ConstantBuffer = -1;
		float Shininess;
	};

	struct RenderObject
	{
		Int32 VertexBuffer = -1;
		Int32 IndexBuffer = -1;
		Int32 ConstantBuffer = -1;
		Diotima::MaterialData Material;
		Matrix4x4 Transform;
	};

	struct MeshData
	{
		std::vector<float> Vertices;
		std::vector<U32> Indices;
	};

	class RenderObjectProxy
	{
		friend class Renderer;

	public:
		~RenderObjectProxy() = default;
		RenderObjectProxy(const RenderObjectProxy&) = delete;

	public:
		void AddMesh(const MeshData& meshData);

	private:
		RenderObjectProxy(RenderObjectProxy&&) = default;
		RenderObjectProxy() = default;

	private:
		// Eventually change this to have the relationship represented differently.
		// Right now a proxy represents an entire mesh as the sum of its parts, but what if
		// we want to do RenderObjectProxy things with the composite objects?
		std::vector<RenderObject> mCompositeObjects;
	};

	class Renderer
	{
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
		
		// #TODO(Josh::Really really don't like this, fix later)
		void SetWindow(void* handle) { mWindowHandle = handle; }

		void EnableVsync(bool bEnable);

		const Vector2D& GetCanvasSize();
		void ResizeCanvas(const Vector2D& newSize);

		void SetViewportSize(const Vector2D& newSize);
		const Vector2D& GetViewportSize();

		void ProcessCommands();

		// #TODO
		// private later
		Int32 CreateVertexBuffer(void* data, size_t size, U32 count);
		Int32 CreateIndexBuffer(void* data, size_t size, U32 count);
		Int32 CreateTextureBuffer2D(void* data, U32 width, U32 height);

		RenderObjectProxy* CreateRenderObjectProxy();

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
	};
}
#endif
