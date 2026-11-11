#include <Rendering/Renderer.h>

#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector4D.h>
#include <Utils/Memory/MemoryUtils.h>
#include <Utils/Platform/Filepath.h>

#include <Rendering/RenderCommand.h>
#include <Rendering/MemArena.h>
#include <Rendering/UIFrameData.h>

#include <Rendering/Driver/GFXDevice.h>
#include <Rendering/Driver/GFXDeviceFactory.h>
#include <Rendering/ImGui/ImGuiRenderer.h>

#include <imgui/imgui.h>

// Each of the arena's two buffers. A frame that needs more spills to the heap (MemArena::Alloc), so this only has to
// cover what frames usually use: in 2026-11 that peaked at 12 MB (Sponza loading) and 3 MB (RenderPerfTest). Windows
// commits all of it up front, and the Debug CRT writes to all of it, so it stays near need.
#define MEM_ARENA_SIZE MemoryUtils::Megabytes(64)

namespace Rendering
{
	RenderThread Renderer::m_renderThread;
	std::shared_ptr<UIFrameData> Renderer::m_uiFrameData;
	int Renderer::m_uiFrameNumber = -1;
	std::vector<TextureBuffer2DHandle> Renderer::m_uiTextures;
	U32 Renderer::m_drawCallCount = 0;
	std::vector<std::shared_ptr<TextureReadback>> Renderer::m_backBufferReadbacks;

	inline void* AllocateCommandData(void* src, size_t dataTypeSize, size_t count)
	{
		if (src == nullptr)
		{
			return src;
		}

		void* dst = MemArena::Alloc(dataTypeSize * count);
		memcpy(dst, src, dataTypeSize * count);

		return dst;
	}

	void Renderer::Initialize(void* windowHandle, bool validate)
	{
		MemArena::InitializeArena(MEM_ARENA_SIZE);

		m_renderThread.Initialize(windowHandle, validate);
	}

	void Renderer::Shutdown()
	{
		m_renderThread.Shutdown();
		m_uiFrameData.reset();
		m_uiTextures.clear();

		MemArena::Shutdown();
	}

	void Renderer::BeginFrame(const char* frameName)
	{
		RenderCommand_BeginFrame* command = MemArena::AllocType<RenderCommand_BeginFrame>();
		command->frameName = frameName;

		m_renderThread.PushCommand(command);
	}

	void Renderer::EndFrame()
	{
		RenderCommand_EndFrame* command = MemArena::AllocType<RenderCommand_EndFrame>();
		m_renderThread.PushCommand(command);
	}

	void Renderer::Begin(const char* drawSetName)
	{
		RenderCommand_Begin* command = MemArena::AllocType<RenderCommand_Begin>();
		command->drawSetName = drawSetName;

		m_renderThread.PushCommand(command);
	}

	void Renderer::End()
	{
		RenderCommand_End* command = MemArena::AllocType<RenderCommand_End>();

		m_renderThread.PushCommand(command);
	}

	void Renderer::DevicePresent()
	{
		// After everything this frame draws, and before Present, which may discard the back buffer (flip model)
		for (const std::shared_ptr<TextureReadback>& readback : m_backBufferReadbacks)
		{
			RenderCommand_ReadBackBuffer* readCommand = MemArena::AllocType<RenderCommand_ReadBackBuffer>();
			readCommand->result = readback;
			m_renderThread.PushCommand(readCommand);
		}
		m_backBufferReadbacks.clear();

		RenderCommand_DevicePresent* command = MemArena::AllocType<RenderCommand_DevicePresent>();
		m_renderThread.PushCommand(command);

		// Returns once the render thread has finished the previous frame and taken this one
		m_renderThread.SignalProcess();
	}

	void Renderer::Submit()
	{
		m_renderThread.SignalProcess();
	}

	void Renderer::SetSyncInterval(U32 interval)
	{
		RenderCommand_SetSyncInterval* command = MemArena::AllocType<RenderCommand_SetSyncInterval>();
		command->interval = interval;

		m_renderThread.PushCommand(command);
	}

	bool Renderer::WaitForPresentSlot(U32 timeoutMS)
	{
		return m_renderThread.WaitForPresentSlot(timeoutMS);
	}

	RenderThreadStats Renderer::GetRenderThreadStats()
	{
		return m_renderThread.GetStats();
	}

	U32 Renderer::TakeDrawCallCount()
	{
		const U32 count = m_drawCallCount;
		m_drawCallCount = 0;
		return count;
	}

	void Renderer::HandleWindowResize(const Vector2D& newSize)
	{
		RenderCommand_HandleWindowResize* command = MemArena::AllocType<RenderCommand_HandleWindowResize>();
		command->size = newSize;

		m_renderThread.PushCommand(command);
	}

	void Renderer::ImGuiRender()
	{
		ImDrawData* drawData = ImGui::GetDrawData();
		// Before the first UI frame there's nothing to draw
		if (drawData == nullptr)
		{
			return;
		}

		// A frame without a UI update has the same draw data as the last UI frame, so it draws that frame's copy again.
		// The render thread may still be drawing the old copy when a new one replaces it, so whichever side lets go of a
		// copy last deletes it, and lets go of the textures it draws.
		if (ImGui::GetFrameCount() != m_uiFrameNumber)
		{
			// ImGui (1.92+) asks the backend to create/update/destroy textures (e.g. the font atlas when new glyphs
			// are rasterized). The render thread runs a frame behind on a cloned ImDrawData, so service those
			// requests here while the render thread is idle rather than racing it on the ImTextureData.
			if (drawData->Textures != nullptr)
			{
				bool hasPendingTextures = false;
				for (ImTextureData* tex : *drawData->Textures)
				{
					hasPendingTextures |= (tex->Status != ImTextureStatus_OK);
				}

				if (hasPendingTextures)
				{
					IImGuiRenderer& imguiRenderer = m_renderThread.GetImGuiRenderer();
					m_renderThread.RunWhileIdle([drawData, &imguiRenderer]()
					{
						for (ImTextureData* tex : *drawData->Textures)
						{
							if (tex->Status != ImTextureStatus_OK)
							{
								imguiRenderer.UpdateTexture(tex);
							}
						}
					});
				}
			}

			m_uiFrameData = std::make_shared<UIFrameData>(*drawData, std::move(m_uiTextures));
			m_uiTextures.clear();
			m_uiFrameNumber = ImGui::GetFrameCount();
		}

		RenderCommand_ImGuiRender* command = MemArena::AllocType<RenderCommand_ImGuiRender>();
		command->uiFrameData = m_uiFrameData;

		m_renderThread.PushCommand(command);
	}

	void Renderer::ImGuiNewFrame()
	{
		m_renderThread.GetImGuiRenderer().NewFrame();
	}

	EGraphicsAPI Renderer::GetGraphicsAPI()
	{
		return GFXDeviceFactory::GetGraphicsAPI();
	}

	void Renderer::KeepTextureForUI(const TextureBuffer2DHandle& texture)
	{
		m_uiTextures.push_back(texture);
	}

	VertexBufferHandle Renderer::CreateVertexBuffer(const void* data, size_t dataTypeSize, size_t count, U32 stride)
	{
		RenderCommand_CreateVertexBuffer* command = MemArena::AllocType<RenderCommand_CreateVertexBuffer>();

		command->bufferPtr = m_renderThread.GetDevice().CreateVertexBuffer();
		command->count = count;
		command->dataTypeSize = dataTypeSize;
		command->stride = stride;

		// Copied into the command, so the caller's data can go once this returns
		command->data = AllocateCommandData(const_cast<void*>(data), dataTypeSize, count);
		
		m_renderThread.PushCommand(command);

		return VertexBufferHandle(command->bufferPtr);
	}

	VertexBufferHandle Renderer::CreateDynamicVertexBuffer(size_t capacityBytes, U32 stride)
	{
		RenderCommand_CreateVertexBuffer* command = MemArena::AllocType<RenderCommand_CreateVertexBuffer>();

		command->bufferPtr = m_renderThread.GetDevice().CreateVertexBuffer();
		command->count = capacityBytes;
		command->dataTypeSize = 1;
		command->stride = stride;
		command->isDynamic = true;

		m_renderThread.PushCommand(command);

		return VertexBufferHandle(command->bufferPtr);
	}

	void Renderer::UpdateVertexBuffer(const VertexBufferHandle& buffer, const void* data, size_t size)
	{
		RenderCommand_UpdateVertexBuffer* command = MemArena::AllocType<RenderCommand_UpdateVertexBuffer>();
		command->bufferHandle = buffer;
		command->size = size;
		command->data = AllocateCommandData(const_cast<void*>(data), size, 1);

		m_renderThread.PushCommand(command);
	}

	IndexBufferHandle Renderer::CreateIndexBuffer(const void* data, size_t dataTypeSize, size_t count)
	{
		RenderCommand_CreateIndexBuffer* command = MemArena::AllocType<RenderCommand_CreateIndexBuffer>();
		command->bufferPtr = m_renderThread.GetDevice().CreateIndexBuffer();
		command->dataTypeSize = dataTypeSize;
		command->count = count;

		// Copied into the command, so the caller's data can go once this returns
		command->data = AllocateCommandData(const_cast<void*>(data), dataTypeSize, count);

		m_renderThread.PushCommand(command);

		return IndexBufferHandle(command->bufferPtr);
	}

	ConstantBufferHandle Renderer::CreateConstantBuffer(void* data, size_t dataTypeSize, U32 alignment, size_t count)
	{
		RenderCommand_CreateConstantBuffer* command = MemArena::AllocType<RenderCommand_CreateConstantBuffer>();
		command->bufferPtr = m_renderThread.GetDevice().CreateConstantBuffer(alignment);
		command->count = count;
		command->data = AllocateCommandData(data, dataTypeSize, count);
		command->dataTypeSize = dataTypeSize;

		m_renderThread.PushCommand(command);

		return ConstantBufferHandle(command->bufferPtr);
	}

	TextureBuffer2DHandle Renderer::CreateTextureBuffer2D(const GFXTextureBufferParams& params)
	{
		return CreateTextureBuffer2D(TexelBuffer(), params);
	}

	TextureBuffer2DHandle Renderer::CreateTextureBuffer2D(TexelBuffer&& texels, const GFXTextureBufferParams& params)
	{
		RenderCommand_CreateTextureBuffer2D* command = MemArena::AllocType<RenderCommand_CreateTextureBuffer2D>();
		command->bufferPtr = m_renderThread.GetDevice().CreateTextureBuffer2D();
		command->texels = std::move(texels);
		command->params = params;

		m_renderThread.PushCommand(command);

		return TextureBuffer2DHandle(command->bufferPtr);
	}

	VertexShaderHandle Renderer::CreateVertexShader(const Filepath& filePath, const ShaderInputLayout& inputLayout)
	{
		RenderCommand_CreateVertexShader* command = MemArena::AllocType<RenderCommand_CreateVertexShader>();
		command->bufferPtr = m_renderThread.GetDevice().CreateVertexShader();
		command->shaderInputLayout = inputLayout;
		command->filepath = filePath;

		m_renderThread.PushCommand(command);

		return VertexShaderHandle(command->bufferPtr);
	}

	PixelShaderHandle Renderer::CreatePixelShader(const Filepath& filePath)
	{
		RenderCommand_CreatePixelShader* command = MemArena::AllocType<RenderCommand_CreatePixelShader>();
		command->bufferPtr = m_renderThread.GetDevice().CreatePixelShader();
		command->filepath = filePath;

		m_renderThread.PushCommand(command);

		return PixelShaderHandle(command->bufferPtr);
	}

	ComputeShaderHandle Renderer::CreateComputeShader(const Filepath& filePath)
	{
		RenderCommand_CreateComputeShader* command = MemArena::AllocType<RenderCommand_CreateComputeShader>();
		command->bufferPtr = m_renderThread.GetDevice().CreateComputeShader();
		command->filepath = filePath;

		m_renderThread.PushCommand(command);

		return ComputeShaderHandle(command->bufferPtr);
	}

	void Renderer::InternalUploadDataToBuffer(const ConstantBufferHandle& buffer, const void* data, size_t dataSize)
	{
		RenderCommand_UploadDataToBuffer* command = MemArena::AllocType<RenderCommand_UploadDataToBuffer>();
		command->bufferHandle = buffer;

		const size_t alignedSize = MemoryUtils::AlignSize(dataSize, buffer.m_buffer->GetAlignment());
		command->data = AllocateCommandData(const_cast<void*>(data), alignedSize, 1);

		m_renderThread.PushCommand(command);
	}

	void Renderer::ReleaseVertexShader(VertexShaderHandle& shaderHandle)
	{
		RenderCommand_ReleaseVertexShader* command = MemArena::AllocType<RenderCommand_ReleaseVertexShader>();
		command->shaderHandle = shaderHandle;

		m_renderThread.PushCommand(command);
	}

	void Renderer::ReleasePixelShader(PixelShaderHandle& shaderHandle)
	{
		RenderCommand_ReleasePixelShader* command = MemArena::AllocType<RenderCommand_ReleasePixelShader>();
		command->shaderHandle = shaderHandle;

		m_renderThread.PushCommand(command);
	}

	void Renderer::ReleaseComputeShader(ComputeShaderHandle& shaderHandle)
	{
		RenderCommand_ReleaseComputeShader* command = MemArena::AllocType<RenderCommand_ReleaseComputeShader>();
		command->shaderHandle = shaderHandle;

		m_renderThread.PushCommand(command);
	}

	void Renderer::ClearDepthStencilBuffer(const TextureBuffer2DHandle& buffer)
	{
		RenderCommand_ClearDepthStencilBuffer* command = MemArena::AllocType<RenderCommand_ClearDepthStencilBuffer>();
		command->bufferHandle = buffer;

		m_renderThread.PushCommand(command);
	}

	void Renderer::ClearDepthStencilSlice(const TextureBuffer2DHandle& buffer, U32 slice)
	{
		RenderCommand_ClearDepthStencilBuffer* command = MemArena::AllocType<RenderCommand_ClearDepthStencilBuffer>();
		command->bufferHandle = buffer;
		command->bEverySlice = false;
		command->slice = slice;

		m_renderThread.PushCommand(command);
	}

	void Renderer::ClearRenderTargets()
	{
		RenderCommand_ClearRenderTargets* command = MemArena::AllocType<RenderCommand_ClearRenderTargets>();

		m_renderThread.PushCommand(command);
	}

	void Renderer::SetRenderTargetBackBuffer()
	{
		RenderCommand_SetRenderTargetBackBuffer* command = MemArena::AllocType<RenderCommand_SetRenderTargetBackBuffer>();

		m_renderThread.PushCommand(command);
	}

	void Renderer::SetDepthTarget(const TextureBuffer2DHandle& depthTexture, U32 slice)
	{
		RenderCommand_SetDepthTarget* command = MemArena::AllocType<RenderCommand_SetDepthTarget>();
		command->depthTexture = depthTexture;
		command->slice = slice;

		m_renderThread.PushCommand(command);
	}

	void Renderer::SetColourTarget(const RenderTargetHandle& colourTarget)
	{
		RenderCommand_SetColourTarget* command = MemArena::AllocType<RenderCommand_SetColourTarget>();
		command->colourTarget = colourTarget;
		command->hasDepth = false;

		m_renderThread.PushCommand(command);
	}

	void Renderer::SetColourTarget(const RenderTargetHandle& colourTarget, const TextureBuffer2DHandle& depthTexture)
	{
		RenderCommand_SetColourTarget* command = MemArena::AllocType<RenderCommand_SetColourTarget>();
		command->colourTarget = colourTarget;
		command->depthTexture = depthTexture;
		command->hasDepth = true;

		m_renderThread.PushCommand(command);
	}

	void Renderer::GenerateMips(const TextureBuffer2DHandle& texture)
	{
		RenderCommand_GenerateMips* command = MemArena::AllocType<RenderCommand_GenerateMips>();
		command->texture = texture;

		m_renderThread.PushCommand(command);
	}

	void Renderer::GenerateMips(const TextureBuffer2DHandle& texture, U32 firstSlice, U32 sliceCount)
	{
		AssertExpr(sliceCount > 0);
		RenderCommand_GenerateMips* command = MemArena::AllocType<RenderCommand_GenerateMips>();
		command->texture = texture;
		command->firstSlice = firstSlice;
		command->sliceCount = sliceCount;

		m_renderThread.PushCommand(command);
	}

	void Renderer::CopyTexture(const TextureBuffer2DHandle& source, const TextureBuffer2DHandle& destination)
	{
		RenderCommand_CopyTexture* command = MemArena::AllocType<RenderCommand_CopyTexture>();
		command->source = source;
		command->destination = destination;

		m_renderThread.PushCommand(command);
	}

	void Renderer::CopyTextureRegion(const TextureBuffer2DHandle& source, U32 sourceX, U32 sourceY, U32 width, U32 height,
		const TextureBuffer2DHandle& destination, U32 destinationX, U32 destinationY)
	{
		RenderCommand_CopyTextureRegion* command = MemArena::AllocType<RenderCommand_CopyTextureRegion>();
		command->source = source;
		command->sourceX = sourceX;
		command->sourceY = sourceY;
		command->width = width;
		command->height = height;
		command->destination = destination;
		command->destinationX = destinationX;
		command->destinationY = destinationY;

		m_renderThread.PushCommand(command);
	}

	void Renderer::CopyTextureSlice(const TextureBuffer2DHandle& source, U32 sourceSlice, const TextureBuffer2DHandle& destination, U32 destinationSlice)
	{
		RenderCommand_CopyTextureSlice* command = MemArena::AllocType<RenderCommand_CopyTextureSlice>();
		command->source = source;
		command->sourceSlice = sourceSlice;
		command->destination = destination;
		command->destinationSlice = destinationSlice;

		m_renderThread.PushCommand(command);
	}

	void Renderer::ReadTexel(const TextureBuffer2DHandle& texture, U32 x, U32 y, std::shared_ptr<TexelReadback> result)
	{
		RenderCommand_ReadTexel* command = MemArena::AllocType<RenderCommand_ReadTexel>();
		command->texture = texture;
		command->x = x;
		command->y = y;
		command->result = std::move(result);

		m_renderThread.PushCommand(command);
	}

	void Renderer::ReadTexture(const TextureBuffer2DHandle& texture, std::shared_ptr<TextureReadback> result)
	{
		RenderCommand_ReadTexture* command = MemArena::AllocType<RenderCommand_ReadTexture>();
		command->texture = texture;
		command->result = std::move(result);

		m_renderThread.PushCommand(command);
	}

	void Renderer::ReadBackBuffer(const std::shared_ptr<TextureReadback>& result)
	{
		m_backBufferReadbacks.push_back(result);
	}

	void Renderer::ClearRenderTarget(const RenderTargetHandle& renderTarget, const Vector4D& colour)
	{
		RenderCommand_ClearRenderTarget* command = MemArena::AllocType<RenderCommand_ClearRenderTarget>();
		command->renderTarget = renderTarget;
		command->colour = colour;

		m_renderThread.PushCommand(command);
	}

	void Renderer::SetViewport(const ViewportParams& viewportParams)
	{
		RenderCommand_SetViewport* command = MemArena::AllocType<RenderCommand_SetViewport>();
		command->params = viewportParams;

		m_renderThread.PushCommand(command);
	}

	void Renderer::SetInputLayout(const VertexShaderHandle& vertexShader)
	{
		RenderCommand_SetInputLayout* command = MemArena::AllocType<RenderCommand_SetInputLayout>();
		command->shaderHandle = vertexShader;

		m_renderThread.PushCommand(command);
	}

	void Renderer::SetPrimitiveTopology(EPrimitiveTopology topologyType)
	{
		RenderCommand_SetPrimitiveTopology* command = MemArena::AllocType<RenderCommand_SetPrimitiveTopology>();
		command->topology = topologyType;

		m_renderThread.PushCommand(command);
	}

	void Renderer::SetRasterizerState(ERasterizerState state)
	{
		RenderCommand_SetRasterizerState* command = MemArena::AllocType<RenderCommand_SetRasterizerState>();
		command->state = state;

		m_renderThread.PushCommand(command);
	}

	void Renderer::SetBlendState(EBlendState state)
	{
		RenderCommand_SetBlendState* command = MemArena::AllocType<RenderCommand_SetBlendState>();
		command->state = state;

		m_renderThread.PushCommand(command);
	}

	void Renderer::SetDepthState(EDepthState state)
	{
		RenderCommand_SetDepthState* command = MemArena::AllocType<RenderCommand_SetDepthState>();
		command->state = state;

		m_renderThread.PushCommand(command);
	}

	void Renderer::SetVertexShader(const VertexShaderHandle& vertexShader)
	{
		RenderCommand_SetVertexShader* command = MemArena::AllocType<RenderCommand_SetVertexShader>();
		command->shaderHandle = vertexShader;

		m_renderThread.PushCommand(command);
	}

	void Renderer::SetPixelShader(const PixelShaderHandle& pixelShader)
	{
		RenderCommand_SetPixelShader* command = MemArena::AllocType<RenderCommand_SetPixelShader>();
		command->shaderHandle = pixelShader;

		m_renderThread.PushCommand(command);
	}

	void Renderer::UnsetPixelShader()
	{
		RenderCommand_UnsetPixelShader* command = MemArena::AllocType<RenderCommand_UnsetPixelShader>();

		m_renderThread.PushCommand(command);
	}

	void Renderer::SetConstantBufferVS(const ConstantBufferHandle& buffer, U32 bufferSlot)
	{
		RenderCommand_SetConstantBufferVS* command = MemArena::AllocType<RenderCommand_SetConstantBufferVS>();
		command->bufferHandle = buffer;
		command->bufferSlot = bufferSlot;

		m_renderThread.PushCommand(command);
	}

	void Renderer::SetConstantBufferPS(const ConstantBufferHandle& buffer, U32 bufferSlot)
	{
		RenderCommand_SetConstantBufferPS* command = MemArena::AllocType<RenderCommand_SetConstantBufferPS>();
		command->bufferHandle = buffer;
		command->bufferSlot = bufferSlot;

		m_renderThread.PushCommand(command);
	}

	void Renderer::SetVertexBuffer(const VertexBufferHandle& buffer, U32 bufferSlot)
	{
		RenderCommand_SetVertexBuffer* command = MemArena::AllocType<RenderCommand_SetVertexBuffer>();
		command->bufferHandle = buffer;
		command->bufferSlot = bufferSlot;

		m_renderThread.PushCommand(command);
	}

	void Renderer::SetIndexBuffer(const IndexBufferHandle& buffer)
	{
		RenderCommand_SetIndexBuffer* command = MemArena::AllocType<RenderCommand_SetIndexBuffer>();
		command->bufferHandle = buffer;

		m_renderThread.PushCommand(command);
	}

	void Renderer::SetTextureResource(const TextureBuffer2DHandle& texture, U32 textureSlot)
	{
		RenderCommand_SetTextureResource* command = MemArena::AllocType<RenderCommand_SetTextureResource>();
		command->bufferHandle = texture;
		command->bufferSlot = textureSlot;

		m_renderThread.PushCommand(command);
	}

	void Renderer::UnsetTextureResource(U32 textureSlot)
	{
		RenderCommand_UnsetTextureResource* command = MemArena::AllocType<RenderCommand_UnsetTextureResource>();
		command->textureSlot = textureSlot;

		m_renderThread.PushCommand(command);
	}

	void Renderer::Draw(const VertexBufferHandle& vertexBuffer, size_t count)
	{
		RenderCommand_Draw* command = MemArena::AllocType<RenderCommand_Draw>();
		command->vertexBuffer = vertexBuffer;
		command->count = count;

		m_renderThread.PushCommand(command);
		++m_drawCallCount;
	}

	void Renderer::DrawIndexed(const IndexBufferHandle& indexBuffer)
	{
		RenderCommand_DrawIndexed* command = MemArena::AllocType<RenderCommand_DrawIndexed>();
		command->indexBuffer = indexBuffer;

		m_renderThread.PushCommand(command);
		++m_drawCallCount;
	}

	void Renderer::DrawIndexedInstanced(const IndexBufferHandle& indexBuffer, U32 instanceCount, U32 startInstance)
	{
		RenderCommand_DrawIndexedInstanced* command = MemArena::AllocType<RenderCommand_DrawIndexedInstanced>();
		command->indexBuffer = indexBuffer;
		command->instanceCount = instanceCount;
		command->startInstance = startInstance;

		m_renderThread.PushCommand(command);
		++m_drawCallCount;
	}

	void Renderer::DrawFullScreenQuad()
	{
		RenderCommand_DrawFullScreenQuad* command = MemArena::AllocType<RenderCommand_DrawFullScreenQuad>();

		m_renderThread.PushCommand(command);
		++m_drawCallCount;
	}

	void Renderer::SetComputeShader(const ComputeShaderHandle& computeShader)
	{
		RenderCommand_SetComputeShader* command = MemArena::AllocType<RenderCommand_SetComputeShader>();
		command->shaderHandle = computeShader;

		m_renderThread.PushCommand(command);
	}

	void Renderer::SetConstantBufferCS(const ConstantBufferHandle& buffer, U32 bufferSlot)
	{
		RenderCommand_SetConstantBufferCS* command = MemArena::AllocType<RenderCommand_SetConstantBufferCS>();
		command->bufferHandle = buffer;
		command->bufferSlot = bufferSlot;

		m_renderThread.PushCommand(command);
	}

	void Renderer::SetTextureResourceCS(const TextureBuffer2DHandle& texture, U32 textureSlot)
	{
		RenderCommand_SetTextureResourceCS* command = MemArena::AllocType<RenderCommand_SetTextureResourceCS>();
		command->bufferHandle = texture;
		command->bufferSlot = textureSlot;

		m_renderThread.PushCommand(command);
	}

	void Renderer::UnsetTextureResourceCS(U32 textureSlot)
	{
		RenderCommand_UnsetTextureResourceCS* command = MemArena::AllocType<RenderCommand_UnsetTextureResourceCS>();
		command->textureSlot = textureSlot;

		m_renderThread.PushCommand(command);
	}

	void Renderer::SetUnorderedAccess(const TextureBuffer2DHandle& texture, U32 slot, U32 mip)
	{
		RenderCommand_SetUnorderedAccess* command = MemArena::AllocType<RenderCommand_SetUnorderedAccess>();
		command->texture = texture;
		command->slot = slot;
		command->mip = mip;

		m_renderThread.PushCommand(command);
	}

	void Renderer::UnsetUnorderedAccess(U32 slot)
	{
		RenderCommand_UnsetUnorderedAccess* command = MemArena::AllocType<RenderCommand_UnsetUnorderedAccess>();
		command->slot = slot;

		m_renderThread.PushCommand(command);
	}

	void Renderer::Dispatch(U32 groupsX, U32 groupsY, U32 groupsZ)
	{
		RenderCommand_Dispatch* command = MemArena::AllocType<RenderCommand_Dispatch>();
		command->groupsX = groupsX;
		command->groupsY = groupsY;
		command->groupsZ = groupsZ;

		m_renderThread.PushCommand(command);
	}
}
