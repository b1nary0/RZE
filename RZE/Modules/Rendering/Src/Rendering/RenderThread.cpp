#include <StdAfx.h>
#include <Rendering/RenderThread.h>

#include <Rendering/MemArena.h>
#include <Rendering/RenderCommand.h>

#include <Rendering/Graphics/RenderTarget.h>

#include <Rendering/Driver/DX11/DX11.h>
#include <Rendering/Driver/DX11/DX11Device.h>
#include <Rendering/Driver/DX11/DX11VertexBuffer.h>
#include <Rendering/Driver/DX11/DX11IndexBuffer.h>
#include <Rendering/Driver/DX11/DX11ConstantBuffer.h>
#include <Rendering/Driver/DX11/DX11TextureBuffer2D.h>
#include <Rendering/Driver/DX11/DX11ShaderTypes.h>

#include <Utils/Conversions.h>
#include <Utils/Memory/MemoryUtils.h>
#include <Utils/DebugUtils/Debug.h>

#include <Optick/optick.h>

#include <ImGui/imgui.h>
#include <imGUI/imgui_impl_dx11.h>
#include <imGUI/imgui_impl_win32.h>

#include <d3d9.h>

#define MEM_ARENA_SIZE MemoryUtils::Megabytes(500)

namespace Rendering
{
	D3D_PRIMITIVE_TOPOLOGY ConvertToDX11TopologyType(EPrimitiveTopology topologyType)
	{
		switch (topologyType)
		{
		case EPrimitiveTopology::TriangleList: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case EPrimitiveTopology::TriangleStrip: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;

		default: return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}
	}

	static bool k_processSignal = false;

	RenderThread::RenderThread()
	{
	}

	void RenderThread::Initialize(void* windowHandle)
	{
		AssertNotNull(windowHandle);

		MemArena::InitializeArena(MEM_ARENA_SIZE);

		m_windowHandle = windowHandle;

		m_device = std::make_unique<DX11Device>();
		m_device->SetWindow(m_windowHandle);
		m_device->Initialize();

		InitializeImGui();
	}

	void RenderThread::Shutdown()
	{
		ImGui_ImplDX11_Shutdown();
		m_device->Shutdown();

		MemArena::Shutdown();
	}

	void RenderThread::InitializeImGui()
	{
		ImGui::CreateContext();
		ImGui_ImplWin32_Init(m_windowHandle);
		ImGui_ImplDX11_Init(&m_device->GetHardwareDevice(), &m_device->GetDeviceContext());
	}

	void RenderThread::Update()
	{
		if (k_processSignal)
		{
			ProcessCommands();

			k_processSignal = false;
		}
	}

	void RenderThread::PushCommand(RenderCommand* command)
	{
		AssertNotNull(command);
		m_commandQueue.push(command);
	}

	void RenderThread::ProcessCommands()
	{
		AssertExpr(k_processSignal = true);
		k_processSignal = false;

		RenderCommand* currCommand = reinterpret_cast<RenderCommand*>(MemArena::GetConsumerPtr());
		while (currCommand != nullptr)
		{
			m_commandQueue.pop();

			switch (currCommand->type)
			{
			case RenderCommandType::ImGuiRender:
			{
				ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_ImGuiRender));

				break;
			}
			case RenderCommandType::BeginFrame:
			{
				RenderCommand_BeginFrame* command = static_cast<RenderCommand_BeginFrame*>(currCommand);

				D3DPERF_BeginEvent(0xffffffff, Conversions::StringToWString(command->frameName).c_str());

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_BeginFrame));

				break;
			}

			case RenderCommandType::EndFrame:
			{
				D3DPERF_EndEvent();

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_EndFrame));
				break;
			}

			case RenderCommandType::Begin:
			{
				RenderCommand_Begin* command = static_cast<RenderCommand_Begin*>(currCommand);

				D3DPERF_BeginEvent(0xffffffff, Conversions::StringToWString(command->drawSetName).c_str());

				// @TODO This is temporary until the API is written for SetRenderTarget() and ClearDepthStencilView()
				// @note past josh wtf api are you talking about
				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.RSSetState(m_device->mRasterState);

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_Begin));

				break;
			}

			case RenderCommandType::End:
			{
				// #TODO form the api such that we can verify Begin() and End() calls for sanity checks
				D3DPERF_EndEvent();

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_End));

				break;
			}

			case RenderCommandType::DevicePresent:
			{
				OPTICK_EVENT("Device Present");
				m_device->Present();

				currCommand = nullptr;

				break;
			}

			case RenderCommandType::HandleWindowResize:
			{
				RenderCommand_HandleWindowResize* command = static_cast<RenderCommand_HandleWindowResize*>(currCommand);

				const Vector2D& newSize = command->size;
				m_device->HandleWindowResize(static_cast<U32>(newSize.X()), static_cast<U32>(newSize.Y()));

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_HandleWindowResize));

				break;
			}

			case RenderCommandType::CreateVertexBuffer:
			{
				RenderCommand_CreateVertexBuffer* command = static_cast<RenderCommand_CreateVertexBuffer*>(currCommand);
				command->bufferPtr->SetDevice(m_device.get());
				command->bufferPtr->Allocate(command->data, command->dataTypeSize, command->count, command->stride);

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_CreateVertexBuffer));

				break;
			}

			case RenderCommandType::CreateIndexBuffer:
			{
				RenderCommand_CreateIndexBuffer* command = static_cast<RenderCommand_CreateIndexBuffer*>(currCommand);
				command->bufferPtr->SetDevice(m_device.get());
				command->bufferPtr->Allocate(command->data, command->dataTypeSize, command->count);

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_CreateIndexBuffer));

				break;
			}

			case RenderCommandType::CreateConstantBuffer:
			{
				RenderCommand_CreateConstantBuffer* command = static_cast<RenderCommand_CreateConstantBuffer*>(currCommand);
				command->bufferPtr->SetDevice(m_device.get());
				command->bufferPtr->Allocate(command->data, command->dataTypeSize, command->count);

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_CreateConstantBuffer));

				break;
			}

			case RenderCommandType::CreateTextureBuffer2D:
			{
				RenderCommand_CreateTextureBuffer2D* command = static_cast<RenderCommand_CreateTextureBuffer2D*>(currCommand);
				command->bufferPtr->SetDevice(m_device.get());
				command->bufferPtr->Allocate(command->data, command->params);

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_CreateTextureBuffer2D));

				break;
			}

			case RenderCommandType::CreateVertexShader:
			{
				RenderCommand_CreateVertexShader* command = static_cast<RenderCommand_CreateVertexShader*>(currCommand);
				command->bufferPtr->SetDevice(m_device.get());
				command->bufferPtr->Create(command->filepath, command->shaderInputLayout);
				
				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_CreateVertexShader));

				break;
			}

			case RenderCommandType::CreatePixelShader:
			{
				RenderCommand_CreatePixelShader* command = static_cast<RenderCommand_CreatePixelShader*>(currCommand);
				command->bufferPtr->SetDevice(m_device.get());
				command->bufferPtr->Create(command->filepath);

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_CreatePixelShader));

				break;
			}

			case RenderCommandType::UploadDataToBuffer:
			{
				RenderCommand_UploadDataToBuffer* command = static_cast<RenderCommand_UploadDataToBuffer*>(currCommand);
				DX11ConstantBuffer* cbuf = static_cast<DX11ConstantBuffer*>(command->bufferHandle.m_buffer.get());
				cbuf->UpdateSubresources(command->data);

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_UploadDataToBuffer));

				break;
			}

			case RenderCommandType::ReleaseVertexShader:
			{
				RenderCommand_ReleaseVertexShader* command = static_cast<RenderCommand_ReleaseVertexShader*>(currCommand);
				command->shaderHandle.m_shader.reset();

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_ReleaseVertexShader));

				break;
			}

			case RenderCommandType::ReleasePixelShader:
			{
				RenderCommand_ReleasePixelShader* command = static_cast<RenderCommand_ReleasePixelShader*>(currCommand);
				command->shaderHandle.m_shader.reset();

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_ReleasePixelShader));

				break;
			}

			case RenderCommandType::ClearDepthStencilBuffer:
			{
				RenderCommand_ClearDepthStencilBuffer* command = static_cast<RenderCommand_ClearDepthStencilBuffer*>(currCommand);
				DX11TextureBuffer2D* texturePtr = static_cast<DX11TextureBuffer2D*>(command->bufferHandle.m_buffer.get());

				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.ClearDepthStencilView(&texturePtr->GetDepthView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_ClearDepthStencilBuffer));

				break;
			}

			case RenderCommandType::SetRenderTarget:
			{
				RenderCommand_SetRenderTarget* command = static_cast<RenderCommand_SetRenderTarget*>(currCommand);
				const RenderTargetTexture* renderTarget = command->renderTarget;

				RenderTargetHandle target = renderTarget->GetTargetPlatformObject();
				TextureBuffer2DHandle depthTexture = renderTarget->GetDepthTexturePlatformObject();

				DX11TextureBuffer2D* targetPtr = static_cast<DX11TextureBuffer2D*>(target.m_buffer.get());
				DX11TextureBuffer2D* depthTexturePtr = static_cast<DX11TextureBuffer2D*>(depthTexture.m_buffer.get());

				ID3D11RenderTargetView* rtv = &targetPtr->GetTargetView();
				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.OMSetRenderTargets(1, &rtv, &depthTexturePtr->GetDepthView());

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_SetRenderTarget));

				break;
			}

			case RenderCommandType::ClearRenderTargets:
			{
				ID3D11RenderTargetView* nullrtv = nullptr;
				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.OMSetRenderTargets(1, &nullrtv, nullptr);

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_ClearRenderTargets));

				break;
			}

			case RenderCommandType::SetRenderTargetBackBuffer:
			{
				constexpr FLOAT s_backBufferClearColour[4] = { 0.25f, 0.45f, 0.65f, 1.0f };

				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.ClearRenderTargetView(m_device->mRenderTargetView, s_backBufferClearColour);
				deviceContext.ClearDepthStencilView(m_device->mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
				deviceContext.OMSetRenderTargets(1, &m_device->mRenderTargetView, m_device->mDepthStencilView);

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_SetRenderTargetBackBuffer));

				break;
			}

			case RenderCommandType::ClearRenderTarget:
			{
				RenderCommand_ClearRenderTarget* command = static_cast<RenderCommand_ClearRenderTarget*>(currCommand);

				std::shared_ptr<ITextureBuffer2D> texture = command->renderTarget.m_buffer;
				DX11TextureBuffer2D* texturePtr = static_cast<DX11TextureBuffer2D*>(texture.get());
				const Vector4D& colour = command->colour;

				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				FLOAT rgba[4] = { colour.X(), colour.Y(), colour.Z(), colour.W() };
				deviceContext.ClearRenderTargetView(&texturePtr->GetTargetView(), rgba);

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_ClearRenderTarget));

				break;
			}

			case RenderCommandType::SetViewport:
			{
				RenderCommand_SetViewport* command = static_cast<RenderCommand_SetViewport*>(currCommand);
				const ViewportParams& params = command->params;

				D3D11_VIEWPORT viewport;
				viewport.Width = params.Width;
				viewport.Height = params.Height;
				viewport.MinDepth = params.MinDepth;
				viewport.MaxDepth = params.MaxDepth;
				viewport.TopLeftX = params.TopLeftX;
				viewport.TopLeftY = params.TopLeftY;

				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.RSSetViewports(1, &viewport);

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_SetViewport));

				break;
			}

			case RenderCommandType::SetInputLayout:
			{
				RenderCommand_SetInputLayout* command = static_cast<RenderCommand_SetInputLayout*>(currCommand);
				command->shaderHandle.m_shader->SetInputLayout();

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_SetInputLayout));

				break;
			}

			case RenderCommandType::SetPrimitiveTopology:
			{
				RenderCommand_SetPrimitiveTopology* command = static_cast<RenderCommand_SetPrimitiveTopology*>(currCommand);

				D3D_PRIMITIVE_TOPOLOGY d3dTopology = ConvertToDX11TopologyType(command->topology);
				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.IASetPrimitiveTopology(d3dTopology);

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_SetPrimitiveTopology));

				break;
			}

			case RenderCommandType::SetVertexShader:
			{
				RenderCommand_SetVertexShader* command = static_cast<RenderCommand_SetVertexShader*>(currCommand);
				command->shaderHandle.m_shader->SetActive();
				
				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_SetVertexShader));

				break;
			}

			case RenderCommandType::SetPixelShader:
			{
				RenderCommand_SetPixelShader* command = static_cast<RenderCommand_SetPixelShader*>(currCommand);
				command->shaderHandle.m_shader->SetActive();

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_SetPixelShader));

				break;
			}

			case RenderCommandType::SetConstantBufferVS:
			{
				RenderCommand_SetConstantBufferVS* command = static_cast<RenderCommand_SetConstantBufferVS*>(currCommand);
				command->bufferHandle.m_buffer->SetActiveVS(command->bufferSlot);

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_SetConstantBufferVS));

				break;
			}

			case RenderCommandType::SetConstantBufferPS:
			{
				RenderCommand_SetConstantBufferPS* command = static_cast<RenderCommand_SetConstantBufferPS*>(currCommand);
				command->bufferHandle.m_buffer->SetActivePS(command->bufferSlot);

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_SetConstantBufferPS));

				break;
			}

			case RenderCommandType::SetVertexBuffer:
			{
				RenderCommand_SetVertexBuffer* command = static_cast<RenderCommand_SetVertexBuffer*>(currCommand);
				command->bufferHandle.m_buffer->SetActive(command->bufferSlot);

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_SetVertexBuffer));

				break;
			}

			case RenderCommandType::SetIndexBuffer:
			{
				RenderCommand_SetIndexBuffer* command = static_cast<RenderCommand_SetIndexBuffer*>(currCommand);
				command->bufferHandle.m_buffer->SetActive();

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_SetIndexBuffer));

				break;
			}

			case RenderCommandType::SetTextureResource:
			{
				RenderCommand_SetTextureResource* command = static_cast<RenderCommand_SetTextureResource*>(currCommand);
				command->bufferHandle.m_buffer->SetActive(command->bufferSlot);

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_SetTextureResource));

				break;
			}

			case RenderCommandType::UnsetTextureResource:
			{
				RenderCommand_UnsetTextureResource* command = static_cast<RenderCommand_UnsetTextureResource*>(currCommand);

				ID3D11ShaderResourceView* nullSRV = nullptr;
				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();

				deviceContext.PSSetShaderResources(command->textureSlot, 1, &nullSRV);

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_UnsetTextureResource));

				break;
			}

			case RenderCommandType::Draw:
			{
				RenderCommand_Draw* command = static_cast<RenderCommand_Draw*>(currCommand);

				std::shared_ptr<IVertexBuffer> bufferPtr = command->vertexBuffer.m_buffer;
				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.Draw(command->count, 0);

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_Draw));

				break;
			}

			case RenderCommandType::DrawIndexed:
			{
				RenderCommand_DrawIndexed* command = static_cast<RenderCommand_DrawIndexed*>(currCommand);
				
				std::shared_ptr<IIndexBuffer> bufferPtr = command->indexBuffer.m_buffer;
				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.DrawIndexed(bufferPtr->GetIndexCount(), 0, 0);

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_DrawIndexed));

				break;
			}

			case RenderCommandType::DrawFullScreenQuad:
			{
				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.Draw(4, 0);

				currCommand = (RenderCommand*)((Byte*)currCommand + sizeof(RenderCommand_DrawFullScreenQuad));

				break;
			}

			default:
			{
				RZE_LOG_ARGS("[Rendering::Renderer] Unknown RenderCommandType encountered: %d", currCommand->type);
				AssertFalse();
			}
			}
		}
	}
	void RenderThread::SignalProcess()
	{
		AssertExpr(k_processSignal == false);
		k_processSignal = true;
	}
}