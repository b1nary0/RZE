#include <Rendering/Renderer.h>

#include <Optick/optick.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector4D.h>
#include <Utils/Platform/FilePath.h>

// DX11
#include <Rendering/Driver/DX11/DX11Device.h>
#include <Rendering/Driver/DX11/DX11.h>
#include <Rendering/Driver/DX11/DX11ConstantBuffer.h>
#include <Rendering/Driver/DX11/DX11IndexBuffer.h>
#include <Rendering/Driver/DX11/DX11VertexBuffer.h>
#include <Rendering/Driver/DX11/DX11TextureBuffer2D.h>
#include <Rendering/Driver/DX11/DX11ShaderTypes.h>

#include <ImGui/imgui.h>
#include <imGUI/imgui_impl_dx11.h>
#include <imGUI/imgui_impl_win32.h>

namespace Rendering
{
	D3D_PRIMITIVE_TOPOLOGY ConvertToDX11TopologyType(EPrimitiveTopology topologyType)
	{
		switch (topologyType)
		{
			case EPrimitiveTopology::TriangleList: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

			default: return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}
	}

	std::unique_ptr<DX11Device> Renderer::m_device = nullptr;
	void* Renderer::m_windowHandle = nullptr;

	Renderer::Renderer()
	{
	}

	Renderer::~Renderer()
	{
	}

	void Renderer::Initialize(void* windowHandle)
	{
		AssertNotNull(windowHandle);

		m_windowHandle = windowHandle;

		m_device = std::make_unique<DX11Device>();
		m_device->SetWindow(m_windowHandle);
		m_device->Initialize();
		
		InitializeImGui();
	}
	
	void Renderer::Shutdown()
	{
		ImGui_ImplDX11_Shutdown();
		m_device->Shutdown();
	}

	void Renderer::BeginFrame()
	{
	}

	void Renderer::EndFrame()
	{
		{
			OPTICK_EVENT("Device Present");
			m_device->Present();
		}
	}

	void Renderer::Begin()
	{
		// @TODO This is temporary until the API is written for SetRenderTarget() and ClearDepthStencilView()
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.RSSetState(m_device->mRasterState);
		deviceContext.OMSetRenderTargets(1, &m_device->mRenderTargetView, m_device->mDepthStencilView);
		deviceContext.ClearDepthStencilView(m_device->mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}

	void Renderer::End()
	{
	}

	void Renderer::InitializeImGui()
	{
		ImGui::CreateContext();
		ImGui_ImplWin32_Init(m_windowHandle);
		ImGui_ImplDX11_Init(&m_device->GetHardwareDevice(), &m_device->GetDeviceContext());
	}

	void Renderer::HandleWindowResize(const Vector2D& newSize)
	{
		m_device->HandleWindowResize(static_cast<U32>(newSize.X()), static_cast<U32>(newSize.Y()));
	}

	VertexBufferHandle Renderer::CreateVertexBuffer(void* data, size_t dataTypeSize, size_t count)
	{
		std::shared_ptr<DX11VertexBuffer> vertexBuffer = std::make_shared<DX11VertexBuffer>();
		vertexBuffer->SetDevice(m_device.get());
		vertexBuffer->Allocate(data, dataTypeSize, count);

		return VertexBufferHandle(vertexBuffer);
	}

	IndexBufferHandle Renderer::CreateIndexBuffer(void* data, size_t dataTypeSize, size_t count)
	{
		std::shared_ptr<DX11IndexBuffer> indexBuffer = std::make_shared<DX11IndexBuffer>();
		indexBuffer->SetDevice(m_device.get());
		indexBuffer->Allocate(data, dataTypeSize, count);

		return IndexBufferHandle(indexBuffer);
	}

	ConstantBufferHandle Renderer::CreateConstantBuffer(void* data, size_t dataTypeSize, size_t count)
	{
		std::shared_ptr<DX11ConstantBuffer> constantBuffer = std::make_shared<DX11ConstantBuffer>();
		constantBuffer->SetDevice(m_device.get());
		constantBuffer->Allocate(data, dataTypeSize, count);

		return ConstantBufferHandle(constantBuffer);
	}

	TextureBuffer2DHandle Renderer::CreateTextureBuffer2D(void* data, const GFXTextureBufferParams& params)
	{
		std::shared_ptr<DX11TextureBuffer2D> textureBuffer = std::make_shared<DX11TextureBuffer2D>();
		textureBuffer->SetDevice(m_device.get());

		if (params.bIsRenderTarget)
		{
			// @TODO Can we remove the need for this?
			textureBuffer->SetIsRenderTarget();
		}

		textureBuffer->Allocate(data, params);

		return TextureBuffer2DHandle(textureBuffer);
	}

	VertexShaderHandle Renderer::CreateVertexShader(const FilePath& filePath, const ShaderInputLayout& inputLayout)
	{
		std::shared_ptr<DX11VertexShader> vertexShader = std::make_shared<DX11VertexShader>();
		vertexShader->SetDevice(m_device.get());
		vertexShader->Create(filePath, inputLayout);

		return VertexShaderHandle(vertexShader);
	}

	PixelShaderHandle Renderer::CreatePixelShader(const FilePath& filePath)
	{
		std::shared_ptr<DX11PixelShader> pixelShader = std::make_shared<DX11PixelShader>();
		pixelShader->SetDevice(m_device.get());
		pixelShader->Create(filePath);

		return PixelShaderHandle(pixelShader);
	}

	void Renderer::UploadDataToBuffer(const ConstantBufferHandle& buffer, const void* data)
	{
		std::shared_ptr<IConstantBuffer> bufferPtr = buffer.m_buffer;
		DX11ConstantBuffer* const dx11Buf = static_cast<DX11ConstantBuffer*>(bufferPtr.get());
		dx11Buf->UpdateSubresources(data);
	}

	void Renderer::ReleaseVertexShader(VertexShaderHandle& shaderHandle)
	{
		shaderHandle.m_shader.reset();
	}

	void Renderer::ReleasePixelShader(PixelShaderHandle& shaderHandle)
	{
		shaderHandle.m_shader.reset();
	}

	void Renderer::SetClearColour(const Vector4D& colour)
	{
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		// @TODO Move render target off device and set it on render state
		FLOAT rgba[4] = { colour.X(), colour.Y(), colour.Z(), colour.W() };
		deviceContext.ClearRenderTargetView(m_device->mRenderTargetView, rgba);
	}

	void Renderer::SetViewport(const ViewportParams& viewportParams)
	{
		D3D11_VIEWPORT viewport;
		viewport.Width = viewportParams.Width;
		viewport.Height = viewportParams.Height;
		viewport.MinDepth = viewportParams.MinDepth;
		viewport.MaxDepth = viewportParams.MaxDepth;
		viewport.TopLeftX = viewportParams.TopLeftX;
		viewport.TopLeftY = viewportParams.TopLeftY;

		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.RSSetViewports(1, &viewport);
	}

	void Renderer::SetInputLayout(const VertexShaderHandle& vertexShader)
	{
		std::shared_ptr<IVertexShader> shaderPtr = vertexShader.m_shader;
		shaderPtr->SetInputLayout();
	}

	void Renderer::SetPrimitiveTopology(EPrimitiveTopology topologyType)
	{
		D3D_PRIMITIVE_TOPOLOGY d3dTopology = ConvertToDX11TopologyType(topologyType);
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.IASetPrimitiveTopology(d3dTopology);
	}

	void Renderer::SetVertexShader(const VertexShaderHandle& vertexShader)
	{
		std::shared_ptr<IVertexShader> shaderPtr = vertexShader.m_shader;
		shaderPtr->SetActive();
	}

	void Renderer::SetPixelShader(const PixelShaderHandle& pixelShader)
	{
		std::shared_ptr<IPixelShader> shaderPtr = pixelShader.m_shader;
		shaderPtr->SetActive();
	}

	void Renderer::SetConstantBufferVS(const ConstantBufferHandle& buffer, U32 bufferSlot)
	{
		std::shared_ptr<IConstantBuffer> bufferPtr = buffer.m_buffer;
		bufferPtr->SetActiveVS(bufferSlot);
	}

	void Renderer::SetConstantBufferPS(const ConstantBufferHandle& buffer, U32 bufferSlot)
	{
		std::shared_ptr<IConstantBuffer> bufferPtr = buffer.m_buffer;
		bufferPtr->SetActivePS(bufferSlot);
	}

	void Renderer::SetVertexBuffer(const VertexBufferHandle& buffer, U32 bufferSlot)
	{
		std::shared_ptr<IVertexBuffer> bufferPtr = buffer.m_buffer;
		bufferPtr->SetActive(bufferSlot);
	}

	void Renderer::SetIndexBuffer(const IndexBufferHandle& buffer)
	{
		std::shared_ptr<IIndexBuffer> bufferPtr = buffer.m_buffer;
		bufferPtr->SetActive();
	}

	void Renderer::SetTextureResource(const TextureBuffer2DHandle& texture, U32 textureSlot)
	{
		std::shared_ptr<ITextureBuffer2D> bufferPtr = texture.m_buffer;
		bufferPtr->SetActive(textureSlot);
	}

	void Renderer::DrawIndexed(const IndexBufferHandle& indexBuffer)
	{
		std::shared_ptr<IIndexBuffer> bufferPtr = indexBuffer.m_buffer;
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.DrawIndexed(bufferPtr->GetIndexCount(), 0, 0);
	}
}
