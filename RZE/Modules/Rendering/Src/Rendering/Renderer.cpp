#include <Rendering/Renderer.h>

#include <Optick/optick.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector4D.h>
#include <Utils/Platform/FilePath.h>

// DX11
#include <Rendering/Driver/DX11/DX11Device.h>
#include <Rendering/Driver/DX11/DX11.h>
#include <Rendering/Driver/DX11/DX11IndexBuffer.h>
#include <Rendering/Driver/DX11/DX11VertexBuffer.h>
#include <Rendering/Driver/DX11/DX11ShaderTypes.h>

namespace Rendering
{
	std::unique_ptr<DX11Device> Renderer::m_device;

	Renderer::Renderer()
	{
	}

	Renderer::~Renderer()
	{
	}

	void Renderer::Initialize(void* windowHandle)
	{
		m_device = std::make_unique<DX11Device>();
		m_device->SetWindow(windowHandle);
		m_device->Initialize();

#ifdef IMGUI_ENABLED
		ImGui::CreateContext();
		ImGui_ImplWin32_Init(windowHandle);
		ImGui_ImplDX11_Init(&m_device->GetHardwareDevice(), &m_device->GetDeviceContext());
#endif
	}
	
	void Renderer::Shutdown()
	{
#ifdef IMGUI_ENABLED
		ImGui_ImplDX11_Shutdown();
#endif
		m_device->Shutdown();
	}

	void Renderer::BeginFrame()
	{
	}

	void Renderer::EndFrame()
	{
		m_device->Present();
	}

	void Renderer::Begin()
	{
		// @TODO This is temporary until the API is written for SetRenderTarget() and ClearDepthStencilView()
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.OMSetRenderTargets(1, &m_device->mRenderTargetView, m_device->mDepthStencilView);
		deviceContext.ClearDepthStencilView(m_device->mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}

	void Renderer::End()
	{
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

	VertexShaderHandle Renderer::CreateVertexShader(const FilePath& filePath)
	{
		std::shared_ptr<DX11VertexShader> vertexShader = std::make_shared<DX11VertexShader>();
		vertexShader->SetDevice(m_device.get());
		vertexShader->Create(filePath);

		return VertexShaderHandle(vertexShader);
	}

	PixelShaderHandle Renderer::CreatePixelShader(const FilePath& filePath)
	{
		std::shared_ptr<DX11PixelShader> pixelShader = std::make_shared<DX11PixelShader>();
		pixelShader->SetDevice(m_device.get());
		pixelShader->Create(filePath);

		return PixelShaderHandle(pixelShader);
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
}
