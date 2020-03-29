#include <Diotima/Driver/DX11/DX11GFXTextureBuffer2D.h>

#include <Diotima/Driver/DX11/DX11GFXDevice.h>

#include <Diotima/Driver/DX11/DX11.h>

#include <Utils/DebugUtils/Debug.h>

namespace Diotima
{

	void DX11GFXTextureBuffer2D::Allocate(void* data, const GFXTextureBufferParams& params)
	{
		D3D11_SAMPLER_DESC sampDesc;
		ZeroMemory(&sampDesc, sizeof(sampDesc));
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
		mDevice->GetHardwareDevice().CreateSamplerState(&sampDesc, &mSamplerState);

		if (params.bIsDepthTexture)
		{
			D3D11_TEXTURE2D_DESC textureDesc;
			ZeroMemory(&textureDesc, sizeof(textureDesc));

			textureDesc.Width = params.Width;
			textureDesc.Height = params.Height;
			textureDesc.MipLevels = 1;
			textureDesc.ArraySize = 1;
			textureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			textureDesc.SampleDesc.Count = 1;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Usage = D3D11_USAGE_DEFAULT;
			textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			textureDesc.CPUAccessFlags = 0;
			textureDesc.MiscFlags = 0;

			HRESULT hr = mDevice->GetHardwareDevice().CreateTexture2D(&textureDesc, NULL, &mResource);
			AssertExpr(hr == S_OK);
			hr = mDevice->GetHardwareDevice().CreateDepthStencilView(mResource, NULL, &mDSV);
			AssertExpr(hr == S_OK);
		}
		else
		{
			D3D11_TEXTURE2D_DESC textureDesc;
			ZeroMemory(&textureDesc, sizeof(textureDesc));

			textureDesc.Width = params.Width;
			textureDesc.Height = params.Height;
			textureDesc.MipLevels = params.MipLevels;
			textureDesc.ArraySize = 1;
			textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			textureDesc.SampleDesc.Count = params.SampleCount;
			textureDesc.SampleDesc.Quality = params.SampleQuality;
			textureDesc.Usage = D3D11_USAGE_DEFAULT;

			UINT bindFlags = 0;
			if (params.bIsShaderResource) bindFlags = (bindFlags | D3D11_BIND_SHADER_RESOURCE);
			if (params.bIsRenderTarget) bindFlags = (bindFlags | D3D11_BIND_RENDER_TARGET);

			textureDesc.BindFlags = bindFlags;
			textureDesc.CPUAccessFlags = 0;
			textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			ZeroMemory(&srvDesc, sizeof(srvDesc));
			srvDesc.Format = textureDesc.Format;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = params.MostDetailedMip;
			srvDesc.Texture2D.MipLevels = (params.MipLevels == 0) ? -1 : params.MipLevels;

			D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
			rtvDesc.Format = textureDesc.Format;
			rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Texture2D.MipSlice = 0;

			HRESULT hr = mDevice->GetHardwareDevice().CreateTexture2D(&textureDesc, NULL, &mResource);
			AssertExpr(hr == S_OK);

			if (data != nullptr)
			{
				U32 rowPitch = (32 * params.Width) / 8;
				mDevice->GetDeviceContext().UpdateSubresource(mResource, 0, NULL, data, rowPitch, 0);
			}

			hr = mDevice->GetHardwareDevice().CreateRenderTargetView(mResource, &rtvDesc, &mRTV);
			AssertExpr(hr == S_OK);
			hr = mDevice->GetHardwareDevice().CreateShaderResourceView(mResource, &srvDesc, &mSRV);
			AssertExpr(hr == S_OK);

			mDevice->GetDeviceContext().GenerateMips(mSRV);
		}
	}

	void DX11GFXTextureBuffer2D::Release()
	{
		AssertNotNull(mResource);
		AssertNotNull(mSRV);
		AssertNotNull(mRTV);
		AssertNotNull(mSamplerState);

		mResource->Release();
		mSRV->Release();
		mRTV->Release();
		mSamplerState->Release();
	}

	void DX11GFXTextureBuffer2D::SetIsRenderTarget()
	{
		bIsRenderTarget = true;
	}

	void DX11GFXTextureBuffer2D::SetDevice(DX11GFXDevice* device)
	{
		AssertNotNull(device);
		mDevice = device;
	}

	ID3D11ShaderResourceView& DX11GFXTextureBuffer2D::GetResourceView()
	{
		AssertNotNull(mSRV);
		return *mSRV;
	}

	ID3D11RenderTargetView& DX11GFXTextureBuffer2D::GetTargetView()
	{
		AssertNotNull(mRTV);
		return *mRTV;
	}

	ID3D11DepthStencilView& DX11GFXTextureBuffer2D::GetDepthView()
	{
		AssertNotNull(mDSV);
		return *mDSV;
	}

	ID3D11SamplerState& DX11GFXTextureBuffer2D::GetSamplerState()
	{
		AssertNotNull(mSamplerState);
		return *mSamplerState;
	}

}