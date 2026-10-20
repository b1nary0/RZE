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
#include <Utils/DebugUtils/Debug.h>

#include <Optick/optick.h>

#include <ImGui/imgui.h>
#include <imGUI/imgui_impl_dx11.h>
#include <imGUI/imgui_impl_win32.h>

#include <d3d9.h>

namespace Rendering
{
	D3D_PRIMITIVE_TOPOLOGY ConvertToDX11TopologyType(EPrimitiveTopology topologyType)
	{
		switch (topologyType)
		{
		case EPrimitiveTopology::TriangleList: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case EPrimitiveTopology::TriangleStrip: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		case EPrimitiveTopology::LineList: return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;

		default: return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}
	}

	namespace
	{
		U32 GetBytesPerTexel(DXGI_FORMAT format)
		{
			switch (format)
			{
			case DXGI_FORMAT_R8G8B8A8_UNORM:
			case DXGI_FORMAT_R16G16_FLOAT:
			case DXGI_FORMAT_R32_UINT:
				return 4;
			case DXGI_FORMAT_R16G16B16A16_FLOAT:
				return 8;

			default:
				AssertMsg(false, "Reading back a texture format with no known texel size");
				return 0;
			}
		}

		// Copies a box of a texture's most detailed mip into a staging texture and maps it, waiting for the GPU to
		// finish drawing it, then calls read(mapped) before unmapping. Reads are rare (a click, a capture), so the
		// staging copy is made for each one rather than kept around.
		template <typename ReadFunc>
		void ReadBackRegion(DX11Device& device, DX11TextureBuffer2D& texture, const D3D11_BOX& box, ReadFunc&& read)
		{
			D3D11_TEXTURE2D_DESC stagingDesc;
			texture.GetHWResource()->GetDesc(&stagingDesc);
			// D3D only copies depth-stencil textures whole, and wants no box for that
			const bool isWhole = box.left == 0 && box.top == 0 && box.right == stagingDesc.Width && box.bottom == stagingDesc.Height;
			stagingDesc.Width = box.right - box.left;
			stagingDesc.Height = box.bottom - box.top;
			stagingDesc.MipLevels = 1;
			stagingDesc.Usage = D3D11_USAGE_STAGING;
			stagingDesc.BindFlags = 0;
			stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
			stagingDesc.MiscFlags = 0;

			ID3D11Texture2D* staging = nullptr;
			HRESULT hr = device.GetHardwareDevice().CreateTexture2D(&stagingDesc, nullptr, &staging);
			AssertExpr(hr == S_OK);

			ID3D11DeviceContext& deviceContext = device.GetDeviceContext();
			deviceContext.CopySubresourceRegion(staging, 0, 0, 0, 0, texture.GetHWResource(), 0, isWhole ? nullptr : &box);

			D3D11_MAPPED_SUBRESOURCE mapped;
			hr = deviceContext.Map(staging, 0, D3D11_MAP_READ, 0, &mapped);
			AssertExpr(hr == S_OK);
			read(mapped);
			deviceContext.Unmap(staging, 0);
			staging->Release();
		}
	}

	RenderThread::RenderThread()
	{
	}

	void RenderThread::Initialize(void* windowHandle)
	{
		AssertNotNull(windowHandle);

		m_windowHandle = windowHandle;

		m_device = std::make_unique<DX11Device>();
		m_device->SetWindow(m_windowHandle);
		m_device->Initialize();
		// Taken before the thread starts, so the main thread can wait on it without touching the device
		m_frameLatencyWaitableObject = m_device->GetFrameLatencyWaitableObject();

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
		{
			// @todo need to wrap this it's ugly and see if we can API-ifize our usage of this pattern in general
			// this block is to ensure the proper shutdown thread synchronization
			std::unique_lock waitlock(m_updateMutex);
			m_updateCondition.wait(waitlock, [this]() { return m_processSignal == false; });
			m_processSignal = true;
			waitlock.unlock();
			m_updateCondition.notify_all();
		}

		m_shuttingDown = true;
		m_thread.join();

		ImGui_ImplDX11_Shutdown();
		m_frameLatencyWaitableObject = nullptr;
		m_device->Shutdown();
	}

	bool RenderThread::WaitForPresentSlot(U32 timeoutMS)
	{
		if (m_frameLatencyWaitableObject == nullptr)
		{
			return true;
		}

		return WaitForSingleObjectEx(m_frameLatencyWaitableObject, timeoutMS, TRUE) == WAIT_OBJECT_0;
	}

	void RenderThread::InitializeImGui()
	{
		ImGui::CreateContext();
		ImGui_ImplWin32_Init(m_windowHandle);
		ImGui_ImplDX11_Init(&m_device->GetHardwareDevice(), &m_device->GetDeviceContext());
	}

	void RenderThread::Update()
	{
		std::unique_lock lock(m_updateMutex);
		{
			OPTICK_EVENT("Wait");
			m_updateCondition.wait(lock, [this] { return m_processSignal; });
		}

		if (!m_shuttingDown)
		{
			ProcessCommands();
		}

		m_processSignal = false;
		lock.unlock();
		m_updateCondition.notify_all();
	}

	void RenderThread::PushCommand(RenderCommand* command)
	{
		AssertNotNull(command);
		m_producerQueue.push_back(command);
	}

	void RenderThread::ProcessCommands()
	{
		OPTICK_EVENT();
		AssertExpr(m_processSignal == true);

		// Marker names are converted to wide strings, so they're only built when a profiler is listening.
		// Checked once per batch: a batch is a whole frame, so every BeginEvent below still gets its EndEvent.
		m_emitProfilerMarkers = D3DPERF_GetStatus() != 0;

		for (RenderCommand* command : m_consumerQueue)
		{
			switch (command->type)
			{
			case RenderCommandType::ImGuiRender:
			{
				RenderCommand_ImGuiRender* cmd = static_cast<RenderCommand_ImGuiRender*>(command);

				// The copy is deleted when the command lets go of it, unless Renderer still holds it for the next frame
				ImGui_ImplDX11_RenderDrawData(cmd->drawData.get());

				break;
			}
			case RenderCommandType::BeginFrame:
			{
				RenderCommand_BeginFrame* cmd = static_cast<RenderCommand_BeginFrame*>(command);

				BeginProfilerEvent(cmd->frameName);

				break;
			}

			case RenderCommandType::EndFrame:
			{
				EndProfilerEvent();

				break;
			}

			case RenderCommandType::Begin:
			{
				RenderCommand_Begin* cmd = static_cast<RenderCommand_Begin*>(command);

				BeginProfilerEvent(cmd->drawSetName);

				// @TODO This is temporary until the API is written for SetRenderTarget() and ClearDepthStencilView()
				// @note past josh wtf api are you talking about
				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.RSSetState(m_device->mRasterState);
				deviceContext.PSSetSamplers(1, 1, &m_device->mShadowSamplerState);

				break;
			}

			case RenderCommandType::End:
			{
				// #TODO form the api such that we can verify Begin() and End() calls for sanity checks
				EndProfilerEvent();

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

			case RenderCommandType::SetSyncInterval:
			{
				RenderCommand_SetSyncInterval* cmd = static_cast<RenderCommand_SetSyncInterval*>(command);
				m_device->SetSyncInterval(cmd->interval);

				break;
			}

			case RenderCommandType::CreateVertexBuffer:
			{
				RenderCommand_CreateVertexBuffer* cmd = static_cast<RenderCommand_CreateVertexBuffer*>(command);
				cmd->bufferPtr->SetDevice(m_device.get());
				cmd->bufferPtr->SetDynamic(cmd->isDynamic);
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
				cmd->bufferPtr->Allocate(cmd->texels.GetTexels(), cmd->params);

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

			case RenderCommandType::UpdateVertexBuffer:
			{
				RenderCommand_UpdateVertexBuffer* cmd = static_cast<RenderCommand_UpdateVertexBuffer*>(command);
				DX11VertexBuffer* vbuf = static_cast<DX11VertexBuffer*>(cmd->bufferHandle.m_buffer.get());

				vbuf->UpdateData(cmd->data, cmd->size);

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

			case RenderCommandType::SetDepthTarget:
			{
				RenderCommand_SetDepthTarget* cmd = static_cast<RenderCommand_SetDepthTarget*>(command);
				DX11TextureBuffer2D* depthTexturePtr = static_cast<DX11TextureBuffer2D*>(cmd->depthTexture.m_buffer.get());

				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.OMSetRenderTargets(0, nullptr, &depthTexturePtr->GetDepthView());

				break;
			}

			case RenderCommandType::SetColourTarget:
			{
				RenderCommand_SetColourTarget* cmd = static_cast<RenderCommand_SetColourTarget*>(command);
				DX11TextureBuffer2D* targetPtr = static_cast<DX11TextureBuffer2D*>(cmd->colourTarget.m_buffer.get());

				ID3D11DepthStencilView* dsv = nullptr;
				if (cmd->hasDepth)
				{
					dsv = &static_cast<DX11TextureBuffer2D*>(cmd->depthTexture.m_buffer.get())->GetDepthView();
				}

				ID3D11RenderTargetView* rtv = &targetPtr->GetTargetView();
				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.OMSetRenderTargets(1, &rtv, dsv);

				break;
			}

			case RenderCommandType::GenerateMips:
			{
				RenderCommand_GenerateMips* cmd = static_cast<RenderCommand_GenerateMips*>(command);
				DX11TextureBuffer2D* texturePtr = static_cast<DX11TextureBuffer2D*>(cmd->texture.m_buffer.get());

				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.GenerateMips(&texturePtr->GetResourceView());

				break;
			}

			case RenderCommandType::ReadTexel:
			{
				RenderCommand_ReadTexel* cmd = static_cast<RenderCommand_ReadTexel*>(command);
				DX11TextureBuffer2D* texturePtr = static_cast<DX11TextureBuffer2D*>(cmd->texture.m_buffer.get());

				const D3D11_BOX texel = { cmd->x, cmd->y, 0, cmd->x + 1, cmd->y + 1, 1 };
				ReadBackRegion(*m_device, *texturePtr, texel, [cmd](const D3D11_MAPPED_SUBRESOURCE& mapped)
					{
						memcpy(&cmd->result->Value, mapped.pData, sizeof(U32));
					});

				cmd->result->IsReady.store(true, std::memory_order_release);

				break;
			}

			case RenderCommandType::ReadTexture:
			{
				RenderCommand_ReadTexture* cmd = static_cast<RenderCommand_ReadTexture*>(command);
				DX11TextureBuffer2D* texturePtr = static_cast<DX11TextureBuffer2D*>(cmd->texture.m_buffer.get());

				D3D11_TEXTURE2D_DESC desc;
				texturePtr->GetHWResource()->GetDesc(&desc);

				TextureReadback& result = *cmd->result;
				result.Width = desc.Width;
				result.Height = desc.Height;
				result.BytesPerTexel = GetBytesPerTexel(desc.Format);

				const size_t rowSize = static_cast<size_t>(result.Width) * result.BytesPerTexel;
				result.Data.resize(rowSize * result.Height);

				const D3D11_BOX whole = { 0, 0, 0, desc.Width, desc.Height, 1 };
				ReadBackRegion(*m_device, *texturePtr, whole, [&result, rowSize](const D3D11_MAPPED_SUBRESOURCE& mapped)
					{
						// Mapped rows can be padded past the texture's width
						const U8* source = static_cast<const U8*>(mapped.pData);
						for (U32 row = 0; row < result.Height; ++row)
						{
							memcpy(&result.Data[row * rowSize], source + static_cast<size_t>(row) * mapped.RowPitch, rowSize);
						}
					});

				result.IsReady.store(true, std::memory_order_release);

				break;
			}

			case RenderCommandType::ClearRenderTarget:
			{
				RenderCommand_ClearRenderTarget* cmd = static_cast<RenderCommand_ClearRenderTarget*>(command);

				DX11TextureBuffer2D* texturePtr = static_cast<DX11TextureBuffer2D*>(cmd->renderTarget.m_buffer.get());
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

			case RenderCommandType::SetRasterizerState:
			{
				RenderCommand_SetRasterizerState* cmd = static_cast<RenderCommand_SetRasterizerState*>(command);

				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.RSSetState(cmd->state == ERasterizerState::ShadowCaster ? m_device->mShadowCasterRasterState : m_device->mRasterState);

				break;
			}

			case RenderCommandType::SetBlendState:
			{
				RenderCommand_SetBlendState* cmd = static_cast<RenderCommand_SetBlendState*>(command);

				ID3D11BlendState* blendState = nullptr;
				switch (cmd->state)
				{
				case EBlendState::AlphaBlend:
					blendState = m_device->mAlphaBlendState;
					break;
				case EBlendState::Premultiplied:
					blendState = m_device->mPremultipliedBlendState;
					break;
				case EBlendState::Additive:
					blendState = m_device->mAdditiveBlendState;
					break;
				case EBlendState::Multiply:
					blendState = m_device->mMultiplyBlendState;
					break;
				default:
					break;
				}

				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.OMSetBlendState(blendState, nullptr, 0xffffffff);

				break;
			}

			case RenderCommandType::SetDepthState:
			{
				RenderCommand_SetDepthState* cmd = static_cast<RenderCommand_SetDepthState*>(command);

				ID3D11DepthStencilState* depthState = nullptr;
				switch (cmd->state)
				{
				case EDepthState::ReadOnly:
					depthState = m_device->mDepthReadOnlyState;
					break;
				case EDepthState::Off:
					depthState = m_device->mDepthOffState;
					break;
				default:
					break;
				}

				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.OMSetDepthStencilState(depthState, 0);

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

				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.Draw(cmd->count, 0);

				break;
			}

			case RenderCommandType::DrawIndexed:
			{
				RenderCommand_DrawIndexed* cmd = static_cast<RenderCommand_DrawIndexed*>(command);
				
				ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
				deviceContext.DrawIndexed(cmd->indexBuffer.m_buffer->GetIndexCount(), 0, 0);

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

			command->~RenderCommand();
		}

		// Keeps the capacity, so steady-state frames don't allocate
		m_consumerQueue.clear();
	}

	void RenderThread::BeginProfilerEvent(const char* name)
	{
		if (m_emitProfilerMarkers)
		{
			D3DPERF_BeginEvent(0xffffffff, Conversions::StringToWString(name).c_str());
		}
	}

	void RenderThread::EndProfilerEvent()
	{
		if (m_emitProfilerMarkers)
		{
			D3DPERF_EndEvent();
		}
	}

	void RenderThread::SignalProcess()
	{
		{
			OPTICK_EVENT("Wait | RenderThread::SignalProcess");
			std::unique_lock waitlock(m_updateMutex);
			m_updateCondition.wait(waitlock, [this]() { return m_processSignal == false; });
		}

		AssertExpr(m_processSignal == false);

		MemArena::Cycle();
		std::swap(m_producerQueue, m_consumerQueue);

		std::lock_guard<std::mutex> lock(m_updateMutex);
		m_processSignal = true;
		m_updateCondition.notify_all();
	}
}