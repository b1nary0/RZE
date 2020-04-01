#include <Diotima/Graphics/RenderPasses/DepthPass.h>

#include <Diotima/Driver/DX11/DX11GFXDevice.h>
#include <Diotima/Driver/DX11/DX11GFXVertexBuffer.h>
#include <Diotima/Driver/DX11/DX11GFXIndexBuffer.h>
#include <Diotima/Driver/DX11/DX11GFXConstantBuffer.h>
#include <Diotima/Driver/DX11/DX11GFXTextureBuffer2D.h>
#include <Diotima/Driver/DX11/DX11.h>

#include <Utils/Conversions.h>
#include <Utils/DebugUtils/Debug.h>
#include <Utils/Platform/FilePath.h>

#include <Optick/optick.h>

namespace
{
	D3D11_INPUT_ELEMENT_DESC k_vertLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT numLayoutElements = ARRAYSIZE(k_vertLayout);
}

namespace Diotima
{
	DepthPass::DepthPass()
		: mDevice(nullptr)
	{
	}

	DepthPass::~DepthPass()
	{
	}

	void DepthPass::Initialize(int width, int height)
	{
		SetDevice(&mRenderer->GetDriverDevice());

		// Shaders
		{
			FilePath vertexShaderPath("Assets/Shaders/Shadows_VS.hlsl");
			FilePath pixelShaderPath("Assets/Shaders/Shadows_PS.hlsl");

			HRESULT hr;
			ID3D10Blob* error;
			hr = D3DCompileFromFile(Conversions::StringToWString(vertexShaderPath.GetAbsolutePath()).c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_0", 0, 0, &mVSBlob, &error);

			if (error)
			{
				char const* message =
					static_cast<char const*>(error->GetBufferPointer());

				// Write the warning to the output window when the program is
				// executing through the Microsoft Visual Studio IDE.
				size_t const length = strlen(message);
				std::wstring output = L"";
				for (size_t i = 0; i < length; ++i)
				{
					output += static_cast<wchar_t>(message[i]);
				}
				output += L'\n';
				OutputDebugString(output.c_str());
			}

			hr = D3DCompileFromFile(Conversions::StringToWString(pixelShaderPath.GetAbsolutePath()).c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_0", 0, 0, &mPSBlob, &error);

			if (error)
			{
				char const* message =
					static_cast<char const*>(error->GetBufferPointer());

				// Write the warning to the output window when the program is
				// executing through the Microsoft Visual Studio IDE.
				size_t const length = strlen(message);
				std::wstring output = L"";
				for (size_t i = 0; i < length; ++i)
				{
					output += static_cast<wchar_t>(message[i]);
				}
				output += L'\n';
				OutputDebugString(output.c_str());
			}

			hr = mDevice->GetHardwareDevice().CreateVertexShader(mVSBlob->GetBufferPointer(), mVSBlob->GetBufferSize(), nullptr, &mVertexShader);
			hr = mDevice->GetHardwareDevice().CreatePixelShader(mPSBlob->GetBufferPointer(), mPSBlob->GetBufferSize(), nullptr, &mPixelShader);
		}

		HRESULT hr;
		hr = mDevice->GetHardwareDevice().CreateInputLayout(k_vertLayout, numLayoutElements, mVSBlob->GetBufferPointer(), mVSBlob->GetBufferSize(), &mVertexLayout);

		mViewProjBuf = mDevice->CreateConstantBuffer(sizeof(Matrix4x4), 1);
		mLightBuf = mDevice->CreateConstantBuffer(sizeof(Renderer::LightItemProtocol), 1);


		GFXTextureBufferParams params;
		params.Width = width;
		params.Height = height;
		params.bIsDepthTexture = true;
		params.bIsRenderTarget = true;
		params.bIsShaderResource = true;
		params.MipLevels = 1;
		params.MostDetailedMip = 0;
		params.SampleCount = 1;
		params.SampleQuality = 0;
		mOutputTextureID = mDevice->CreateTextureBuffer2D(nullptr, params);
		mOutputTexture = mDevice->GetTextureBuffer2D(mOutputTextureID);
	}

	void DepthPass::Begin()
	{
		const std::vector<Renderer::LightItemProtocol>& lights = mRenderer->GetLights();
		PrepareLights(lights);

		ID3D11DeviceContext& deviceContext = mDevice->GetDeviceContext();
		ID3D11RenderTargetView* nullViews[] = { nullptr };
		deviceContext.OMSetRenderTargets(1, nullViews, &mOutputTexture->GetDepthView());
		deviceContext.ClearDepthStencilView(&mOutputTexture->GetDepthView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		deviceContext.VSSetShader(mVertexShader, 0, 0);
		deviceContext.PSSetShader(mPixelShader, 0, 0);

		DX11GFXConstantBuffer* const lightBuf = mDevice->GetConstantBuffer(mLightBuf);
		ID3D11Buffer* hwBuf = &lightBuf->GetHardwareBuffer();
		deviceContext.VSSetConstantBuffers(0, 1, &hwBuf);

		deviceContext.IASetInputLayout(mVertexLayout);

		D3D11_VIEWPORT viewport;
		viewport.Width = 4096;
		viewport.Height = 4096;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		deviceContext.RSSetViewports(1, &viewport);
	}

	void DepthPass::Execute()
	{
		OPTICK_EVENT();

		Begin();
		{
			ID3D11DeviceContext& deviceContext = mDevice->GetDeviceContext();
			const std::vector<Renderer::RenderItemDrawCall>& drawCalls = mRenderer->GetDrawCalls();
			for (const Renderer::RenderItemDrawCall& drawCall : drawCalls)
			{
				DX11GFXVertexBuffer* const vertBuf = mDevice->GetVertexBuffer(drawCall.VertexBuffer);
				DX11GFXIndexBuffer* const indexBuf = mDevice->GetIndexBuffer(drawCall.IndexBuffer);

				ID3D11Buffer* hwVertBuf = &vertBuf->GetHardwareBuffer();

				DX11GFXConstantBuffer* modelMatBuf = mDevice->GetConstantBuffer(drawCall.ConstantBuffer);
				ID3D11Buffer* hwModelMatBuf = &modelMatBuf->GetHardwareBuffer();
				deviceContext.VSSetConstantBuffers(1, 1, &hwModelMatBuf);

				struct TempDataLayoutStructure
				{
					Vector3D position;
					Vector3D normal;
					Vector2D uv;
					Vector3D tangents;
				};

				UINT stride = sizeof(TempDataLayoutStructure);
				UINT offset = 0;
				deviceContext.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				deviceContext.IASetVertexBuffers(0, 1, &hwVertBuf, &stride, &offset);
				deviceContext.IASetIndexBuffer(&indexBuf->GetHardwareBuffer(), DXGI_FORMAT_R32_UINT, 0);

				deviceContext.DrawIndexed(indexBuf->GetIndexCount(), 0, 0);
			}
		}
		End();
	}

	void DepthPass::End()
	{
	
	}

	void DepthPass::SetRenderer(Renderer* renderer)
	{
		mRenderer = renderer;
	}

	void DepthPass::SetDevice(DX11GFXDevice* device)
	{
		AssertNotNull(device);
		AssertExpr(mDevice == nullptr);

		mDevice = device;
	}

	void DepthPass::PrepareLights(const std::vector<Renderer::LightItemProtocol>& lights)
	{
		//OPTICK_EVENT();
 		DX11GFXConstantBuffer* const lightBuf = mDevice->GetConstantBuffer(mLightBuf);
 
 		AssertMsg(lights.size() == 1, "First pass shadows only support one light"); 
		lightBuf->UpdateSubresources(&lights[0].LightSpaceMatrix);
	}

	U32 DepthPass::GetOutputBuffer()
	{
		return mOutputTextureID;
	}

}

