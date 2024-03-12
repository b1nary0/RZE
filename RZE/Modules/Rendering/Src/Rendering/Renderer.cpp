#include <Rendering/Renderer.h>


#include <Utils/Conversions.h>
#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector4D.h>
#include <Utils/Memory/MemoryUtils.h>
#include <Utils/Platform/Filepath.h>

// DX11
#include <Rendering/RenderCommand.h>
#include <Rendering/MemArena.h>

#include <Rendering/Driver/DX11/DX11.h>
#include <Rendering/Driver/DX11/DX11Device.h>
#include <Rendering/Driver/DX11/DX11ConstantBuffer.h>
#include <Rendering/Driver/DX11/DX11IndexBuffer.h>
#include <Rendering/Driver/DX11/DX11VertexBuffer.h>
#include <Rendering/Driver/DX11/DX11TextureBuffer2D.h>
#include <Rendering/Driver/DX11/DX11ShaderTypes.h>

#include <Rendering/Graphics/RenderTarget.h>

#include <Optick/optick.h>
#include <imGUI/imgui.h>

#define MEM_ARENA_SIZE MemoryUtils::Kilobytes(512)

namespace Rendering
{
	RenderThread Renderer::m_renderThread;

	Renderer::Renderer()
	{
	}

	Renderer::~Renderer()
	{
	}

	void Renderer::Initialize(void* windowHandle)
	{
		MemArena::InitializeArena(MEM_ARENA_SIZE);

		m_renderThread.Initialize(windowHandle);
	}

	void Renderer::Shutdown()
	{
		m_renderThread.Shutdown();

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
		RenderCommand_DevicePresent* command = MemArena::AllocType<RenderCommand_DevicePresent>();
		m_renderThread.PushCommand(command);

		m_renderThread.SignalProcess();
	}

	void Renderer::HandleWindowResize(const Vector2D& newSize)
	{
		RenderCommand_HandleWindowResize* command = MemArena::AllocType<RenderCommand_HandleWindowResize>();
		command->size = newSize;

		m_renderThread.PushCommand(command);
	}

	ImDrawData* CloneDrawData(ImDrawData* toClone)
	{
		ImDrawData* clone = new ImDrawData();
		clone->Valid = toClone->Valid;
		clone->CmdListsCount = toClone->CmdListsCount;
		clone->TotalIdxCount = toClone->TotalIdxCount;
		clone->TotalVtxCount = toClone->TotalVtxCount;
		clone->DisplayPos = toClone->DisplayPos;
		clone->DisplaySize = toClone->DisplaySize;
		clone->FramebufferScale = toClone->FramebufferScale;

		clone->CmdLists = toClone->CmdLists;
		for (int i = 0; i < toClone->CmdListsCount; ++i)
		{
			clone->CmdLists[i] = toClone->CmdLists[i]->CloneOutput();
		}

		clone->OwnerViewport = toClone->OwnerViewport;

		return clone;
	}

	void Renderer::ImGuiRender()
	{
		RenderCommand_ImGuiRender* command = MemArena::AllocType<RenderCommand_ImGuiRender>();
		command->drawData = CloneDrawData(ImGui::GetDrawData());

		m_renderThread.PushCommand(command);
	}

	VertexBufferHandle Renderer::CreateVertexBuffer(void* data, size_t dataTypeSize, size_t count, U32 stride)
	{
		RenderCommand_CreateVertexBuffer* command = MemArena::AllocType<RenderCommand_CreateVertexBuffer>();

		command->bufferPtr = std::make_shared<DX11VertexBuffer>();
		command->count = count;
		command->dataTypeSize = dataTypeSize;
		command->stride = stride;
		command->data = data;
		
		m_renderThread.PushCommand(command);

		return VertexBufferHandle(command->bufferPtr);
	}

	IndexBufferHandle Renderer::CreateIndexBuffer(void* data, size_t dataTypeSize, size_t count)
	{
		RenderCommand_CreateIndexBuffer* command = MemArena::AllocType<RenderCommand_CreateIndexBuffer>();
		command->bufferPtr = std::make_shared<DX11IndexBuffer>();
		command->dataTypeSize = dataTypeSize;
		command->count = count;
		command->data = data;

		m_renderThread.PushCommand(command);

		return IndexBufferHandle(command->bufferPtr);
	}

	ConstantBufferHandle Renderer::CreateConstantBuffer(void* data, size_t dataTypeSize, U32 alignment, size_t count)
	{
		RenderCommand_CreateConstantBuffer* command = MemArena::AllocType<RenderCommand_CreateConstantBuffer>();
		command->bufferPtr = std::make_shared<DX11ConstantBuffer>(alignment);
		command->count = count;
		command->data = data;
		command->dataTypeSize = dataTypeSize;

		m_renderThread.PushCommand(command);

		return ConstantBufferHandle(command->bufferPtr);
	}

	TextureBuffer2DHandle Renderer::CreateTextureBuffer2D(const void* data, const GFXTextureBufferParams& params)
	{
		RenderCommand_CreateTextureBuffer2D* command = MemArena::AllocType<RenderCommand_CreateTextureBuffer2D>();
		command->bufferPtr = std::make_shared<DX11TextureBuffer2D>();
		command->data = data;
		command->params = params;

		m_renderThread.PushCommand(command);

		return TextureBuffer2DHandle(command->bufferPtr);
	}

	VertexShaderHandle Renderer::CreateVertexShader(const Filepath& filePath, const ShaderInputLayout& inputLayout)
	{
		RenderCommand_CreateVertexShader* command = MemArena::AllocType<RenderCommand_CreateVertexShader>();
		command->bufferPtr = std::make_shared<DX11VertexShader>();
		command->shaderInputLayout = inputLayout;
		command->filepath = filePath;

		m_renderThread.PushCommand(command);

		return VertexShaderHandle(command->bufferPtr);
	}

	PixelShaderHandle Renderer::CreatePixelShader(const Filepath& filePath)
	{
		RenderCommand_CreatePixelShader* command = MemArena::AllocType<RenderCommand_CreatePixelShader>();
		command->bufferPtr = std::make_shared<DX11PixelShader>();
		command->filepath = filePath;

		m_renderThread.PushCommand(command);

		return PixelShaderHandle(command->bufferPtr);
	}

	void Renderer::InternalUploadDataToBuffer(const ConstantBufferHandle& buffer, const void* data, size_t dataSize)
	{
		RenderCommand_UploadDataToBuffer* command = MemArena::AllocType<RenderCommand_UploadDataToBuffer>();
		command->bufferHandle = buffer;

		const size_t alignedSize = MemoryUtils::AlignSize(dataSize, buffer.m_buffer->GetAlignment());
		command->data = malloc(alignedSize);
		memcpy(const_cast<void*>(command->data), data, alignedSize);

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

	void Renderer::ClearDepthStencilBuffer(const TextureBuffer2DHandle& buffer)
	{
		RenderCommand_ClearDepthStencilBuffer* command = MemArena::AllocType<RenderCommand_ClearDepthStencilBuffer>();
		command->bufferHandle = buffer;

		m_renderThread.PushCommand(command);
	}

	// @todo why are we using const RenderTargetTexture* here instead of RenderTargetHandle?
	void Renderer::SetRenderTarget(const RenderTargetTexture* renderTarget)
	{
		if (renderTarget != nullptr)
		{
			RenderCommand_SetRenderTarget* command = MemArena::AllocType<RenderCommand_SetRenderTarget>();
			command->renderTarget = renderTarget;

			m_renderThread.PushCommand(command);
		}
		else
		{
			RenderCommand_ClearRenderTargets* command = MemArena::AllocType<RenderCommand_ClearRenderTargets>();

			m_renderThread.PushCommand(command);
		}
	}

	void Renderer::SetRenderTargetBackBuffer()
	{
		RenderCommand_SetRenderTargetBackBuffer* command = MemArena::AllocType<RenderCommand_SetRenderTargetBackBuffer>();

		m_renderThread.PushCommand(command);
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
	}

	void Renderer::DrawIndexed(const IndexBufferHandle& indexBuffer)
	{
		RenderCommand_DrawIndexed* command = MemArena::AllocType<RenderCommand_DrawIndexed>();
		command->indexBuffer = indexBuffer;

		m_renderThread.PushCommand(command);
	}

	void Renderer::DrawFullScreenQuad()
	{
		RenderCommand_DrawFullScreenQuad* command = MemArena::AllocType<RenderCommand_DrawFullScreenQuad>();

		m_renderThread.PushCommand(command);
	}
}
