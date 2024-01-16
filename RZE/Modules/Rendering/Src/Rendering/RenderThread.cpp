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

		auto threadExec = [this]()
			{
				OPTICK_THREAD("Render Thread");
				while (!m_shuttingDown)
				{
					Update();
				}
			};

		m_thread = std::thread(threadExec);
	}

	void RenderThread::Shutdown()
	{
		m_shuttingDown = true;
		m_thread.join();

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
		if (m_processSignal)
		{
			std::lock_guard lock(m_updateMutex);

			MemArena::Cycle();
			std::swap(m_producerQueue, m_consumerQueue);

			ProcessCommands();

			m_processSignal = false;
		}
	}

	void RenderThread::PushCommand(RenderCommand* command)
	{
		AssertNotNull(command);
		m_producerQueue.push(command);
	}

	void RenderThread::ProcessCommands()
	{
		AssertExpr(m_processSignal = true);

		while (!m_consumerQueue.empty())
		{
			RenderCommand* command = m_consumerQueue.front();
			m_consumerQueue.pop();

			switch (command->type)
			{
			case RenderCommandType::ImGuiRender:
			{
				RenderCommand_ImGuiRender* cmd = static_cast<RenderCommand_ImGuiRender*>(command);

				ImGui_ImplDX11_RenderDrawData(cmd->drawData);

				for (int i = 0; i < cmd->drawData->CmdListsCount; ++i)
				{
					IM_DELETE<ImDrawList>(cmd->drawData->CmdLists[i]);
				}
				delete cmd->drawData;

				break;
			}
			case RenderCommandType::BeginFrame:
			{
				RenderCommand_BeginFrame* cmd = static_cast<RenderCommand_BeginFrame*>(command);

				D3DPERF_BeginEvent(0xffffffff, Conversions::StringToWString(cmd->frameName).c_str());

				break;
			}

			case RenderCommandType::EndFrame:
			{
				D3DPERF_EndEvent();

				break;
			}

			case RenderCommandType::Begin:
			{
				RenderCommand_Begin* cmd = static_cast<RenderCommand_Begin*>(command);

				D3DPERF_BeginEvent(0xffffffff, Conversions::StringToWString(cmd->drawSetName).c_str());

				// @TODO This is temporary until the API is written for SetRenderTarget() and ClearDepthStencilView()
				// @note past josh wtf api are you talking about
				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.RSSetState(m_device->mRasterState);

				break;
			}

			case RenderCommandType::End:
			{
				// #TODO form the api such that we can verify Begin() and End() calls for sanity checks
				D3DPERF_EndEvent();

				break;
			}

			case RenderCommandType::DevicePresent:
			{
				OPTICK_EVENT("Device Present");
				m_device->Present();

				break;
			}

			case RenderCommandType::HandleWindowResize:
			{
				RenderCommand_HandleWindowResize* cmd = static_cast<RenderCommand_HandleWindowResize*>(command);

				const Vector2D& newSize = cmd->size;
				m_device->HandleWindowResize(static_cast<U32>(newSize.X()), static_cast<U32>(newSize.Y()));

				break;
			}

			case RenderCommandType::CreateVertexBuffer:
			{
				RenderCommand_CreateVertexBuffer* cmd = static_cast<RenderCommand_CreateVertexBuffer*>(command);
				cmd->bufferPtr->SetDevice(m_device.get());
				cmd->bufferPtr->Allocate(cmd->data, cmd->dataTypeSize, cmd->count, cmd->stride);

				break;
			}

			case RenderCommandType::CreateIndexBuffer:
			{
				RenderCommand_CreateIndexBuffer* cmd = static_cast<RenderCommand_CreateIndexBuffer*>(command);
				cmd->bufferPtr->SetDevice(m_device.get());
				cmd->bufferPtr->Allocate(cmd->data, cmd->dataTypeSize, cmd->count);

				break;
			}

			case RenderCommandType::CreateConstantBuffer:
			{
				RenderCommand_CreateConstantBuffer* cmd = static_cast<RenderCommand_CreateConstantBuffer*>(command);
				cmd->bufferPtr->SetDevice(m_device.get());
				cmd->bufferPtr->Allocate(cmd->data, cmd->dataTypeSize, cmd->count);

				break;
			}

			case RenderCommandType::CreateTextureBuffer2D:
			{
				RenderCommand_CreateTextureBuffer2D* cmd = static_cast<RenderCommand_CreateTextureBuffer2D*>(command);
				cmd->bufferPtr->SetDevice(m_device.get());
				cmd->bufferPtr->Allocate(cmd->data, cmd->params);

				break;
			}

			case RenderCommandType::CreateVertexShader:
			{
				RenderCommand_CreateVertexShader* cmd = static_cast<RenderCommand_CreateVertexShader*>(command);
				cmd->bufferPtr->SetDevice(m_device.get());
				cmd->bufferPtr->Create(cmd->filepath, cmd->shaderInputLayout);
				
				break;
			}

			case RenderCommandType::CreatePixelShader:
			{
				RenderCommand_CreatePixelShader* cmd = static_cast<RenderCommand_CreatePixelShader*>(command);
				cmd->bufferPtr->SetDevice(m_device.get());
				cmd->bufferPtr->Create(cmd->filepath);

				break;
			}

			case RenderCommandType::UploadDataToBuffer:
			{
				RenderCommand_UploadDataToBuffer* cmd = static_cast<RenderCommand_UploadDataToBuffer*>(command);
				DX11ConstantBuffer* cbuf = static_cast<DX11ConstantBuffer*>(cmd->bufferHandle.m_buffer.get());
				cbuf->UpdateSubresources(cmd->data);

				break;
			}

			case RenderCommandType::ReleaseVertexShader:
			{
				RenderCommand_ReleaseVertexShader* cmd = static_cast<RenderCommand_ReleaseVertexShader*>(command);
				cmd->shaderHandle.m_shader.reset();

				break;
			}

			case RenderCommandType::ReleasePixelShader:
			{
				RenderCommand_ReleasePixelShader* cmd = static_cast<RenderCommand_ReleasePixelShader*>(command);
				cmd->shaderHandle.m_shader.reset();

				break;
			}

			case RenderCommandType::ClearDepthStencilBuffer:
			{
				RenderCommand_ClearDepthStencilBuffer* cmd = static_cast<RenderCommand_ClearDepthStencilBuffer*>(command);
				DX11TextureBuffer2D* texturePtr = static_cast<DX11TextureBuffer2D*>(cmd->bufferHandle.m_buffer.get());

				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.ClearDepthStencilView(&texturePtr->GetDepthView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

				break;
			}

			case RenderCommandType::SetRenderTarget:
			{
				RenderCommand_SetRenderTarget* cmd = static_cast<RenderCommand_SetRenderTarget*>(command);
				const RenderTargetTexture* renderTarget = cmd->renderTarget;

				RenderTargetHandle target = renderTarget->GetTargetPlatformObject();
				TextureBuffer2DHandle depthTexture = renderTarget->GetDepthTexturePlatformObject();

				DX11TextureBuffer2D* targetPtr = static_cast<DX11TextureBuffer2D*>(target.m_buffer.get());
				DX11TextureBuffer2D* depthTexturePtr = static_cast<DX11TextureBuffer2D*>(depthTexture.m_buffer.get());

				ID3D11RenderTargetView* rtv = &targetPtr->GetTargetView();
				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.OMSetRenderTargets(1, &rtv, &depthTexturePtr->GetDepthView());

				break;
			}

			case RenderCommandType::ClearRenderTargets:
			{
				ID3D11RenderTargetView* nullrtv = nullptr;
				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.OMSetRenderTargets(1, &nullrtv, nullptr);

				break;
			}

			case RenderCommandType::SetRenderTargetBackBuffer:
			{
				constexpr FLOAT s_backBufferClearColour[4] = { 0.25f, 0.45f, 0.65f, 1.0f };

				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.ClearRenderTargetView(m_device->mRenderTargetView, s_backBufferClearColour);
				deviceContext.ClearDepthStencilView(m_device->mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
				deviceContext.OMSetRenderTargets(1, &m_device->mRenderTargetView, m_device->mDepthStencilView);

				break;
			}

			case RenderCommandType::ClearRenderTarget:
			{
				RenderCommand_ClearRenderTarget* cmd = static_cast<RenderCommand_ClearRenderTarget*>(command);

				std::shared_ptr<ITextureBuffer2D> texture = cmd->renderTarget.m_buffer;
				DX11TextureBuffer2D* texturePtr = static_cast<DX11TextureBuffer2D*>(texture.get());
				const Vector4D& colour = cmd->colour;

				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				FLOAT rgba[4] = { colour.X(), colour.Y(), colour.Z(), colour.W() };
				deviceContext.ClearRenderTargetView(&texturePtr->GetTargetView(), rgba);

				break;
			}

			case RenderCommandType::SetViewport:
			{
				RenderCommand_SetViewport* cmd = static_cast<RenderCommand_SetViewport*>(command);
				const ViewportParams& params = cmd->params;

				D3D11_VIEWPORT viewport;
				viewport.Width = params.Width;
				viewport.Height = params.Height;
				viewport.MinDepth = params.MinDepth;
				viewport.MaxDepth = params.MaxDepth;
				viewport.TopLeftX = params.TopLeftX;
				viewport.TopLeftY = params.TopLeftY;

				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.RSSetViewports(1, &viewport);

				break;
			}

			case RenderCommandType::SetInputLayout:
			{
				RenderCommand_SetInputLayout* cmd = static_cast<RenderCommand_SetInputLayout*>(command);
				cmd->shaderHandle.m_shader->SetInputLayout();

				break;
			}

			case RenderCommandType::SetPrimitiveTopology:
			{
				RenderCommand_SetPrimitiveTopology* cmd = static_cast<RenderCommand_SetPrimitiveTopology*>(command);

				D3D_PRIMITIVE_TOPOLOGY d3dTopology = ConvertToDX11TopologyType(cmd->topology);
				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.IASetPrimitiveTopology(d3dTopology);

				break;
			}

			case RenderCommandType::SetVertexShader:
			{
				RenderCommand_SetVertexShader* cmd = static_cast<RenderCommand_SetVertexShader*>(command);
				cmd->shaderHandle.m_shader->SetActive();
				
				break;
			}

			case RenderCommandType::SetPixelShader:
			{
				RenderCommand_SetPixelShader* cmd = static_cast<RenderCommand_SetPixelShader*>(command);
				cmd->shaderHandle.m_shader->SetActive();

				break;
			}

			case RenderCommandType::SetConstantBufferVS:
			{
				RenderCommand_SetConstantBufferVS* cmd = static_cast<RenderCommand_SetConstantBufferVS*>(command);
				cmd->bufferHandle.m_buffer->SetActiveVS(cmd->bufferSlot);

				break;
			}

			case RenderCommandType::SetConstantBufferPS:
			{
				RenderCommand_SetConstantBufferPS* cmd = static_cast<RenderCommand_SetConstantBufferPS*>(command);
				cmd->bufferHandle.m_buffer->SetActivePS(cmd->bufferSlot);

				break;
			}

			case RenderCommandType::SetVertexBuffer:
			{
				RenderCommand_SetVertexBuffer* cmd = static_cast<RenderCommand_SetVertexBuffer*>(command);
				cmd->bufferHandle.m_buffer->SetActive(cmd->bufferSlot);

				break;
			}

			case RenderCommandType::SetIndexBuffer:
			{
				RenderCommand_SetIndexBuffer* cmd = static_cast<RenderCommand_SetIndexBuffer*>(command);
				cmd->bufferHandle.m_buffer->SetActive();

				break;
			}

			case RenderCommandType::SetTextureResource:
			{
				RenderCommand_SetTextureResource* cmd = static_cast<RenderCommand_SetTextureResource*>(command);
				cmd->bufferHandle.m_buffer->SetActive(cmd->bufferSlot);

				break;
			}

			case RenderCommandType::UnsetTextureResource:
			{
				RenderCommand_UnsetTextureResource* cmd = static_cast<RenderCommand_UnsetTextureResource*>(command);

				ID3D11ShaderResourceView* nullSRV = nullptr;
				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();

				deviceContext.PSSetShaderResources(cmd->textureSlot, 1, &nullSRV);

				break;
			}

			case RenderCommandType::Draw:
			{
				RenderCommand_Draw* cmd = static_cast<RenderCommand_Draw*>(command);

				std::shared_ptr<IVertexBuffer> bufferPtr = cmd->vertexBuffer.m_buffer;
				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.Draw(cmd->count, 0);

				break;
			}

			case RenderCommandType::DrawIndexed:
			{
				RenderCommand_DrawIndexed* cmd = static_cast<RenderCommand_DrawIndexed*>(command);
				
				std::shared_ptr<IIndexBuffer> bufferPtr = cmd->indexBuffer.m_buffer;
				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.DrawIndexed(bufferPtr->GetIndexCount(), 0, 0);

				break;
			}

			case RenderCommandType::DrawFullScreenQuad:
			{
				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.Draw(4, 0);

				break;
			}

			default:
			{
				RZE_LOG_ARGS("[Rendering::Renderer] Unknown RenderCommandType encountered: %d", command->type);
				AssertFalse();
			}
			}
		}
	}

	void RenderThread::SignalProcess()
	{
		std::lock_guard lock(m_updateMutex);

		//AssertExpr(m_processSignal == false);
		m_processSignal = true;
	}
}