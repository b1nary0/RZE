#include <StdAfx.h>
#include <Rendering/RenderThread.h>

#include <Rendering/RenderCommand.h>
#include <Rendering/TexelReadback.h>
#include <Rendering/TextureReadback.h>
#include <Rendering/UIFrameData.h>

#include <Rendering/Driver/DX11/DX11.h>
#include <Rendering/Driver/DX11/DX11ConstantBuffer.h>
#include <Rendering/Driver/DX11/DX11Device.h>
#include <Rendering/Driver/DX11/DX11IndexBuffer.h>
#include <Rendering/Driver/DX11/DX11ShaderTypes.h>
#include <Rendering/Driver/DX11/DX11TextureBuffer2D.h>
#include <Rendering/Driver/DX11/DX11VertexBuffer.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/TimeUtils.h>

#include <Optick/optick.h>

#include <imGUI/imgui_impl_dx11.h>

#include <cstring>

// What each render command does on the render thread: RenderThread::ProcessCommands calls the Execute for its type
namespace Rendering
{
	namespace
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

		U32 GetBytesPerTexel(DXGI_FORMAT format)
		{
			switch (format)
			{
			case DXGI_FORMAT_R8_UNORM:
				return 1;
			case DXGI_FORMAT_R16_FLOAT:
				return 2;
			case DXGI_FORMAT_R8G8B8A8_UNORM:
			case DXGI_FORMAT_R16G16_FLOAT:
			case DXGI_FORMAT_R32_UINT:
			case DXGI_FORMAT_R32_FLOAT:
			// Depth-stencil targets that can also be read by shaders (DX11TextureBuffer2D)
			case DXGI_FORMAT_R24G8_TYPELESS:
				return 4;
			case DXGI_FORMAT_R16G16B16A16_FLOAT:
			case DXGI_FORMAT_R32G32_UINT:
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

	void RenderThread::Execute(RenderCommand_ImGuiRender& command)
	{
		// The copy is deleted when the command lets go of it, unless Renderer still holds it for the next frame
		ImGui_ImplDX11_RenderDrawData(command.uiFrameData->GetDrawData());
	}

	void RenderThread::Execute(RenderCommand_BeginFrame& command)
	{
		BeginProfilerEvent(command.frameName);
	}

	void RenderThread::Execute(RenderCommand_EndFrame&)
	{
		EndProfilerEvent();
	}

	void RenderThread::Execute(RenderCommand_Begin& command)
	{
		BeginProfilerEvent(command.drawSetName);

		// @TODO This is temporary until the API is written for SetRenderTarget() and ClearDepthStencilView()
		// @note past josh wtf api are you talking about
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.RSSetState(m_device->mRasterState);
		deviceContext.PSSetSamplers(1, 1, &m_device->mShadowSamplerState);
		deviceContext.CSSetSamplers(1, 1, &m_device->mShadowSamplerState);
		deviceContext.PSSetSamplers(2, 1, &m_device->mLinearClampSamplerState);
		deviceContext.CSSetSamplers(2, 1, &m_device->mLinearClampSamplerState);
	}

	void RenderThread::Execute(RenderCommand_End&)
	{
		// #TODO form the api such that we can verify Begin() and End() calls for sanity checks
		EndProfilerEvent();
	}

	void RenderThread::Execute(RenderCommand_DevicePresent&)
	{
		OPTICK_EVENT("Device Present");
		const double presentStart = TimeUtils::GetSeconds();
		m_device->Present();
		m_framePresentTime += TimeUtils::GetSeconds() - presentStart;
	}

	void RenderThread::Execute(RenderCommand_HandleWindowResize& command)
	{
		const Vector2D& newSize = command.size;
		m_device->HandleWindowResize(static_cast<U32>(newSize.X()), static_cast<U32>(newSize.Y()));
	}

	void RenderThread::Execute(RenderCommand_SetSyncInterval& command)
	{
		m_device->SetSyncInterval(command.interval);
	}

	void RenderThread::Execute(RenderCommand_CreateVertexBuffer& command)
	{
		command.bufferPtr->SetDevice(m_device.get());
		command.bufferPtr->SetDynamic(command.isDynamic);
		command.bufferPtr->Allocate(command.data, command.dataTypeSize, command.count, command.stride);
	}

	void RenderThread::Execute(RenderCommand_CreateIndexBuffer& command)
	{
		command.bufferPtr->SetDevice(m_device.get());
		command.bufferPtr->Allocate(command.data, command.dataTypeSize, command.count);
	}

	void RenderThread::Execute(RenderCommand_CreateConstantBuffer& command)
	{
		command.bufferPtr->SetDevice(m_device.get());
		command.bufferPtr->Allocate(command.data, command.dataTypeSize, command.count);
	}

	void RenderThread::Execute(RenderCommand_CreateTextureBuffer2D& command)
	{
		command.bufferPtr->SetDevice(m_device.get());
		command.bufferPtr->Allocate(command.texels.GetTexels(), command.params);
	}

	void RenderThread::Execute(RenderCommand_CreateVertexShader& command)
	{
		command.bufferPtr->SetDevice(m_device.get());
		command.bufferPtr->Create(command.filepath, command.shaderInputLayout);
	}

	void RenderThread::Execute(RenderCommand_CreatePixelShader& command)
	{
		command.bufferPtr->SetDevice(m_device.get());
		command.bufferPtr->Create(command.filepath);
	}

	void RenderThread::Execute(RenderCommand_CreateComputeShader& command)
	{
		command.bufferPtr->SetDevice(m_device.get());
		command.bufferPtr->Create(command.filepath);
	}

	void RenderThread::Execute(RenderCommand_UploadDataToBuffer& command)
	{
		DX11ConstantBuffer* cbuf = static_cast<DX11ConstantBuffer*>(command.bufferHandle.m_buffer.get());

		cbuf->UpdateSubresources(command.data);
	}

	void RenderThread::Execute(RenderCommand_UpdateVertexBuffer& command)
	{
		DX11VertexBuffer* vbuf = static_cast<DX11VertexBuffer*>(command.bufferHandle.m_buffer.get());

		vbuf->UpdateData(command.data, command.size);
	}

	void RenderThread::Execute(RenderCommand_ReleaseVertexShader& command)
	{
		command.shaderHandle.m_shader.reset();
	}

	void RenderThread::Execute(RenderCommand_ReleasePixelShader& command)
	{
		command.shaderHandle.m_shader.reset();
	}

	void RenderThread::Execute(RenderCommand_ReleaseComputeShader& command)
	{
		command.shaderHandle.m_shader.reset();
	}

	void RenderThread::Execute(RenderCommand_ClearDepthStencilBuffer& command)
	{
		DX11TextureBuffer2D* texturePtr = static_cast<DX11TextureBuffer2D*>(command.bufferHandle.m_buffer.get());

		ID3D11DepthStencilView& depthView = command.bEverySlice ? texturePtr->GetDepthView() : texturePtr->GetDepthSliceView(command.slice);
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.ClearDepthStencilView(&depthView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}

	void RenderThread::Execute(RenderCommand_ClearRenderTargets&)
	{
		ID3D11RenderTargetView* nullrtv = nullptr;
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.OMSetRenderTargets(1, &nullrtv, nullptr);
	}

	void RenderThread::Execute(RenderCommand_SetRenderTargetBackBuffer&)
	{
		constexpr FLOAT s_backBufferClearColour[4] = { 0.25f, 0.45f, 0.65f, 1.0f };

		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.ClearRenderTargetView(m_device->mRenderTargetView, s_backBufferClearColour);
		deviceContext.ClearDepthStencilView(m_device->mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		deviceContext.OMSetRenderTargets(1, &m_device->mRenderTargetView, m_device->mDepthStencilView);
	}

	void RenderThread::Execute(RenderCommand_SetDepthTarget& command)
	{
		DX11TextureBuffer2D* depthTexturePtr = static_cast<DX11TextureBuffer2D*>(command.depthTexture.m_buffer.get());

		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.OMSetRenderTargets(0, nullptr, &depthTexturePtr->GetDepthSliceView(command.slice));
	}

	void RenderThread::Execute(RenderCommand_SetColourTarget& command)
	{
		DX11TextureBuffer2D* targetPtr = static_cast<DX11TextureBuffer2D*>(command.colourTarget.m_buffer.get());

		ID3D11DepthStencilView* dsv = nullptr;
		if (command.hasDepth)
		{
			dsv = &static_cast<DX11TextureBuffer2D*>(command.depthTexture.m_buffer.get())->GetDepthView();
		}

		ID3D11RenderTargetView* rtv = &targetPtr->GetTargetView();
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.OMSetRenderTargets(1, &rtv, dsv);
	}

	void RenderThread::Execute(RenderCommand_GenerateMips& command)
	{
		DX11TextureBuffer2D* texturePtr = static_cast<DX11TextureBuffer2D*>(command.texture.m_buffer.get());

		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.GenerateMips(&texturePtr->GetResourceView());
	}

	void RenderThread::Execute(RenderCommand_CopyTexture& command)
	{
		DX11TextureBuffer2D* sourcePtr = static_cast<DX11TextureBuffer2D*>(command.source.m_buffer.get());
		DX11TextureBuffer2D* destinationPtr = static_cast<DX11TextureBuffer2D*>(command.destination.m_buffer.get());

		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.CopyResource(destinationPtr->GetHWResource(), sourcePtr->GetHWResource());
	}

	void RenderThread::Execute(RenderCommand_ClearRenderTarget& command)
	{
		DX11TextureBuffer2D* texturePtr = static_cast<DX11TextureBuffer2D*>(command.renderTarget.m_buffer.get());
		const Vector4D& colour = command.colour;

		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		FLOAT rgba[4] = { colour.X(), colour.Y(), colour.Z(), colour.W() };
		deviceContext.ClearRenderTargetView(&texturePtr->GetTargetView(), rgba);
	}

	void RenderThread::Execute(RenderCommand_SetViewport& command)
	{
		const ViewportParams& params = command.params;

		D3D11_VIEWPORT viewport;
		viewport.Width = params.Width;
		viewport.Height = params.Height;
		viewport.MinDepth = params.MinDepth;
		viewport.MaxDepth = params.MaxDepth;
		viewport.TopLeftX = params.TopLeftX;
		viewport.TopLeftY = params.TopLeftY;

		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.RSSetViewports(1, &viewport);
	}

	void RenderThread::Execute(RenderCommand_SetInputLayout& command)
	{
		command.shaderHandle.m_shader->SetInputLayout();
	}

	void RenderThread::Execute(RenderCommand_SetPrimitiveTopology& command)
	{
		D3D_PRIMITIVE_TOPOLOGY d3dTopology = ConvertToDX11TopologyType(command.topology);
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.IASetPrimitiveTopology(d3dTopology);
	}

	void RenderThread::Execute(RenderCommand_SetRasterizerState& command)
	{
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.RSSetState(m_device->GetRasterizerState(command.state));
	}

	void RenderThread::Execute(RenderCommand_SetBlendState& command)
	{
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.OMSetBlendState(m_device->GetBlendState(command.state), nullptr, 0xffffffff);
	}

	void RenderThread::Execute(RenderCommand_SetDepthState& command)
	{
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.OMSetDepthStencilState(m_device->GetDepthState(command.state), 0);
	}

	void RenderThread::Execute(RenderCommand_SetVertexShader& command)
	{
		command.shaderHandle.m_shader->SetActive();
	}

	void RenderThread::Execute(RenderCommand_SetPixelShader& command)
	{
		command.shaderHandle.m_shader->SetActive();
	}

	void RenderThread::Execute(RenderCommand_UnsetPixelShader&)
	{
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.PSSetShader(nullptr, nullptr, 0);
	}

	void RenderThread::Execute(RenderCommand_SetComputeShader& command)
	{
		command.shaderHandle.m_shader->SetActive();
	}

	void RenderThread::Execute(RenderCommand_SetConstantBufferVS& command)
	{
		command.bufferHandle.m_buffer->SetActiveVS(command.bufferSlot);
	}

	void RenderThread::Execute(RenderCommand_SetConstantBufferPS& command)
	{
		command.bufferHandle.m_buffer->SetActivePS(command.bufferSlot);
	}

	void RenderThread::Execute(RenderCommand_SetConstantBufferCS& command)
	{
		command.bufferHandle.m_buffer->SetActiveCS(command.bufferSlot);
	}

	void RenderThread::Execute(RenderCommand_SetVertexBuffer& command)
	{
		command.bufferHandle.m_buffer->SetActive(command.bufferSlot);
	}

	void RenderThread::Execute(RenderCommand_SetIndexBuffer& command)
	{
		command.bufferHandle.m_buffer->SetActive();
	}

	void RenderThread::Execute(RenderCommand_SetTextureResource& command)
	{
		command.bufferHandle.m_buffer->SetActive(command.bufferSlot);
	}

	void RenderThread::Execute(RenderCommand_UnsetTextureResource& command)
	{
		ID3D11ShaderResourceView* nullSRV = nullptr;
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();

		deviceContext.PSSetShaderResources(command.textureSlot, 1, &nullSRV);
	}

	void RenderThread::Execute(RenderCommand_SetTextureResourceCS& command)
	{
		command.bufferHandle.m_buffer->SetActiveCS(command.bufferSlot);
	}

	void RenderThread::Execute(RenderCommand_UnsetTextureResourceCS& command)
	{
		ID3D11ShaderResourceView* nullSRV = nullptr;
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();

		deviceContext.CSSetShaderResources(command.textureSlot, 1, &nullSRV);
	}

	void RenderThread::Execute(RenderCommand_SetUnorderedAccess& command)
	{
		DX11TextureBuffer2D* texturePtr = static_cast<DX11TextureBuffer2D*>(command.texture.m_buffer.get());

		ID3D11UnorderedAccessView* uav = &texturePtr->GetUnorderedAccessView(command.mip);
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.CSSetUnorderedAccessViews(command.slot, 1, &uav, nullptr);
	}

	void RenderThread::Execute(RenderCommand_UnsetUnorderedAccess& command)
	{
		ID3D11UnorderedAccessView* nullUAV = nullptr;
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.CSSetUnorderedAccessViews(command.slot, 1, &nullUAV, nullptr);
	}

	void RenderThread::Execute(RenderCommand_Draw& command)
	{
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.Draw(command.count, 0);
	}

	void RenderThread::Execute(RenderCommand_DrawIndexed& command)
	{
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.DrawIndexed(command.indexBuffer.m_buffer->GetIndexCount(), 0, 0);
	}

	void RenderThread::Execute(RenderCommand_DrawFullScreenQuad&)
	{
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.Draw(4, 0);
	}

	void RenderThread::Execute(RenderCommand_Dispatch& command)
	{
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.Dispatch(command.groupsX, command.groupsY, command.groupsZ);
	}

	void RenderThread::Execute(RenderCommand_ReadTexel& command)
	{
		DX11TextureBuffer2D* texturePtr = static_cast<DX11TextureBuffer2D*>(command.texture.m_buffer.get());

		D3D11_TEXTURE2D_DESC desc;
		texturePtr->GetHWResource()->GetDesc(&desc);
		const U32 bytesPerTexel = GetBytesPerTexel(desc.Format);
		AssertExpr(bytesPerTexel <= sizeof(command.result->Value));

		const D3D11_BOX texel = { command.x, command.y, 0, command.x + 1, command.y + 1, 1 };
		ReadBackRegion(*m_device, *texturePtr, texel, [&command, bytesPerTexel](const D3D11_MAPPED_SUBRESOURCE& mapped)
			{
				command.result->Value = 0;
				memcpy(&command.result->Value, mapped.pData, bytesPerTexel);
			});

		command.result->IsReady.store(true, std::memory_order_release);
	}

	void RenderThread::Execute(RenderCommand_ReadTexture& command)
	{
		DX11TextureBuffer2D* texturePtr = static_cast<DX11TextureBuffer2D*>(command.texture.m_buffer.get());

		D3D11_TEXTURE2D_DESC desc;
		texturePtr->GetHWResource()->GetDesc(&desc);

		TextureReadback& result = *command.result;
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
	}
}
