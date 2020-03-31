#include <Diotima/Graphics/RenderPasses/ForwardPass.h>

#include <Diotima/Driver/DX11/DX11GFXDevice.h>
#include <Diotima/Driver/DX11/DX11GFXConstantBuffer.h>
#include <Diotima/Driver/DX11/DX11GFXTextureBuffer2D.h>
#include <Diotima/Driver/DX11/DX11GFXIndexBuffer.h>
#include <Diotima/Driver/DX11/DX11GFXVertexBuffer.h>

#include <Diotima/Driver/DX11/DX11.h>

#include <Diotima/Graphics/RenderTarget.h>

#include <Utils/Conversions.h>
#include <Utils/DebugUtils/Debug.h>
#include <Utils/Platform/FilePath.h>
#include <Utils/MemoryUtils.h>

#include <Optick/optick.h>

#include <array>

namespace Diotima
{
	D3D11_INPUT_ELEMENT_DESC k_vertLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT numLayoutElements = ARRAYSIZE(k_vertLayout);

	ForwardPass::ForwardPass()
		: mDevice(nullptr)
	{
	}

	ForwardPass::~ForwardPass()
	{
		// #TODO(Move these and make a proper shutdown/release procedure for GFXRenderPass)
		mVertexShader->Release();
		mPixelShader->Release();
		mVSBlob->Release();
		mPSBlob->Release();
	}

	void ForwardPass::Initialize(int width, int height)
	{
		OPTICK_EVENT();

		SetDevice(mRenderer->mDevice.get());

		mViewProjBuf = mDevice->CreateConstantBuffer(sizeof(Matrix4x4), 1);
		mLightBuf = mDevice->CreateConstantBuffer(sizeof(Renderer::LightItemProtocol), 1);
		mCameraDataBuf = mDevice->CreateConstantBuffer(MemoryUtils::AlignSize(sizeof(Vector3D), 15), 1);

		// Shaders
		{
			FilePath vertexShaderPath("Assets/Shaders/ForwardLighting_VS.hlsl");
			FilePath pixelShaderPath("Assets/Shaders/ForwardLighting_PS.hlsl");

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
	}

	void ForwardPass::Begin()
	{
		const std::vector<Renderer::LightItemProtocol>& lights = mRenderer->GetLights();
		PrepareLights(lights);

		const Renderer::CameraItemProtocol& camera = mRenderer->GetCamera();

		Matrix4x4 camViewProjMat = camera.ProjectionMat * camera.ViewMat;
		DX11GFXConstantBuffer* viewProjBuf = mDevice->GetConstantBuffer(mViewProjBuf);
		viewProjBuf->UpdateSubresources(&camViewProjMat);

		DX11GFXConstantBuffer* camDataBuf = mDevice->GetConstantBuffer(mCameraDataBuf);
		camDataBuf->UpdateSubresources(&camera.Position);
	}

	void ForwardPass::Execute()
	{
		OPTICK_EVENT();

		Begin();
		{
			FLOAT rgba[4] = { 0.35f, 0.35f, 0.5f, 1.0f };
			ID3D11DeviceContext& deviceContext = mDevice->GetDeviceContext();

			RenderTargetTexture* renderTarget = mRenderer->GetRenderTarget();
			if (renderTarget != nullptr)
			{
				DX11GFXTextureBuffer2D* rtBuf = &renderTarget->GetGFXTexture();
				DX11GFXTextureBuffer2D* depthBuf = &renderTarget->GetDepthTexture();
				ID3D11RenderTargetView* hwRTV = &rtBuf->GetTargetView();

				deviceContext.OMSetRenderTargets(1, &hwRTV, &depthBuf->GetDepthView());

				deviceContext.ClearDepthStencilView(&depthBuf->GetDepthView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
				deviceContext.ClearRenderTargetView(hwRTV, rgba);

				const Vector2D& viewportDims = mRenderer->GetViewportSize();
				D3D11_VIEWPORT viewport;
				viewport.Width = viewportDims.X();
				viewport.Height = viewportDims.Y();
				viewport.MinDepth = 0.0f;
				viewport.MaxDepth = 1.0f;
				viewport.TopLeftX = 0.0f;
				viewport.TopLeftY = 0.0f;
				deviceContext.RSSetViewports(1, &viewport);
			}
			else
			{
				deviceContext.OMSetRenderTargets(1, &mDevice->mRenderTargetView, mDevice->mDepthStencilView);

				deviceContext.ClearDepthStencilView(mDevice->mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
				deviceContext.ClearRenderTargetView(mDevice->mRenderTargetView, rgba);
			}

 			deviceContext.RSSetState(mDevice->mRasterState);
 
 			deviceContext.VSSetShader(mVertexShader, 0, 0);
 			deviceContext.PSSetShader(mPixelShader, 0, 0);
 
 			DX11GFXConstantBuffer* viewProjBuf = mDevice->GetConstantBuffer(mViewProjBuf);
 			ID3D11Buffer* vpbHardwareBuf = &viewProjBuf->GetHardwareBuffer();
 			deviceContext.VSSetConstantBuffers(0, 1, &vpbHardwareBuf);
 
 			DX11GFXConstantBuffer* lightBuf = mDevice->GetConstantBuffer(mLightBuf);
 			ID3D11Buffer* hwLightBuf = &lightBuf->GetHardwareBuffer();
 			deviceContext.PSSetConstantBuffers(0, 1, &hwLightBuf);
 
 			DX11GFXConstantBuffer* camDataBuf = mDevice->GetConstantBuffer(mCameraDataBuf);
 			ID3D11Buffer* hwCamDataBuf = &camDataBuf->GetHardwareBuffer();
 			deviceContext.PSSetConstantBuffers(1, 1, &hwCamDataBuf);
 
 			mDevice->GetDeviceContext().IASetInputLayout(mVertexLayout);
 
 			const std::vector<Renderer::RenderItemDrawCall> drawCalls = mRenderer->GetDrawCalls();
 			for (const Renderer::RenderItemDrawCall& drawCall : drawCalls)
 			{
 				// try to draw something
 				mDevice->GetDeviceContext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
 
 				DX11GFXConstantBuffer* modelMatBuf = mDevice->GetConstantBuffer(drawCall.ConstantBuffer);
 				ID3D11Buffer* hwModelMatBuf = &modelMatBuf->GetHardwareBuffer();
 				deviceContext.VSSetConstantBuffers(1, 1, &hwModelMatBuf);
 
 				DX11GFXConstantBuffer* materialBuf = mDevice->GetConstantBuffer(drawCall.MaterialDataBuffer);
 				ID3D11Buffer* hwMaterialBuf = &materialBuf->GetHardwareBuffer();
 				deviceContext.PSSetConstantBuffers(2, 1, &hwMaterialBuf);
 
 				ID3D11Buffer* vertBuf = &mDevice->GetVertexBuffer(drawCall.VertexBuffer)->GetHardwareBuffer();
 
 				struct TempDataLayoutStructure
 				{
 					Vector3D position;
 					Vector3D normal;
 					Vector2D uv;
 					Vector3D tangents;
 				};
 				UINT stride = sizeof(TempDataLayoutStructure);
 				UINT offset = 0;
 				mDevice->GetDeviceContext().IASetVertexBuffers(0, 1, &vertBuf, &stride, &offset);
 
 				// Index buffer
 				DX11GFXIndexBuffer* indexBuf = mDevice->GetIndexBuffer(drawCall.IndexBuffer);
 				mDevice->GetDeviceContext().IASetIndexBuffer(&indexBuf->GetHardwareBuffer(), DXGI_FORMAT_R32_UINT, 0);
 
 				std::array<DX11GFXTextureBuffer2D*, 3> textureArray = {
 					mDevice->GetTextureBuffer2D(drawCall.TextureSlot0),
 					mDevice->GetTextureBuffer2D(drawCall.TextureSlot1),
 					mDevice->GetTextureBuffer2D(drawCall.TextureSlot2)
 				};
 
 				for (size_t texBufIdx = 0; texBufIdx < textureArray.size(); ++texBufIdx)
 				{
 					ID3D11ShaderResourceView* resourceView = &textureArray[texBufIdx]->GetResourceView();
 					ID3D11SamplerState* samplerState = &textureArray[texBufIdx]->GetSamplerState();
 					deviceContext.PSSetShaderResources(texBufIdx, 1, &resourceView);
 					deviceContext.PSSetSamplers(texBufIdx, 1, &samplerState);
 				}
 
 				deviceContext.DrawIndexed(indexBuf->GetIndexCount(), 0, 0);
 			}
		}
		End();
	}

	void ForwardPass::End()
	{
	}

	void ForwardPass::SetRenderer(Renderer* renderer)
	{
		mRenderer = renderer;
	}

	void ForwardPass::SetDevice(DX11GFXDevice* device)
	{
		AssertNotNull(device);
		AssertExpr(mDevice == nullptr);

		mDevice = device;
	}

	void ForwardPass::PrepareLights(const std::vector<Renderer::LightItemProtocol>& lights)
	{
		//OPTICK_EVENT();

		DX11GFXConstantBuffer* const lightBuf = mDevice->GetConstantBuffer(mLightBuf);
		lightBuf->UpdateSubresources(lights.data());
	}

	void ForwardPass::OnWindowResize(int newWidth, int newHeight)
	{
// 		mDepthStencilBuffer.release();
// 
// 		mDepthStencilBuffer = std::make_unique<DX12GFXDepthStencilBuffer>();
// 		mDepthStencilBuffer->SetSize(newWidth, newHeight);
// 		mDepthStencilBuffer->SetDevice(mDevice);
// 		mDepthStencilBuffer->Allocate();
// 
// 		mViewport->Width = static_cast<float>(newWidth);
// 		mViewport->Height = static_cast<float>(newHeight);
// 		mViewport->TopLeftX = 0;
// 		mViewport->TopLeftY = 0;
// 		mViewport->MinDepth = 0.0f;
// 		mViewport->MaxDepth = 1.0f;
// 
// 		mScissorRect.left = 0;
// 		mScissorRect.top = 0;
// 		mScissorRect.right = newWidth;
// 		mScissorRect.bottom = newHeight;
	}

}

