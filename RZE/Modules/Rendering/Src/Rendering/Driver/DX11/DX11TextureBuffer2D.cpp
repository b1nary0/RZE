#include <Rendering/Driver/DX11/DX11TextureBuffer2D.h>

#include <Rendering/Driver/DX11/DX11Device.h>

#include <Rendering/Driver/DX11/DX11.h>

#include <Utils/DebugUtils/Debug.h>

namespace Rendering
{

	void DX11TextureBuffer2D::Allocate(void* data, const GFXTextureBufferParams& params)
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

		// #TODO
		// lol get rid of this shit
		if (params.bIsDepthTexture)
		{
			bIsDepthTexture = true;

			D3D11_TEXTURE2D_DESC textureDesc;
			ZeroMemory(&textureDesc, sizeof(textureDesc));

			textureDesc.Width = params.Width;
			textureDesc.Height = params.Height;
			textureDesc.MipLevels = 1;
			textureDesc.ArraySize = 1;
			textureDesc.Format = (params.bIsShaderResource) ? DXGI_FORMAT_R24G8_TYPELESS : DXGI_FORMAT_D24_UNORM_S8_UINT;
			textureDesc.SampleDesc.Count = 1;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Usage = D3D11_USAGE_DEFAULT;
			textureDesc.BindFlags = (params.bIsShaderResource) ? D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE : D3D11_BIND_DEPTH_STENCIL;
			textureDesc.CPUAccessFlags = 0;
			textureDesc.MiscFlags = 0;

			HRESULT hr = mDevice->GetHardwareDevice().CreateTexture2D(&textureDesc, NULL, &mResource);
			AssertExpr(hr == S_OK);

			D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
			ZeroMemory(&depthStencilViewDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
			depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			depthStencilViewDesc.Texture2D.MipSlice = 0;
			hr = mDevice->GetHardwareDevice().CreateDepthStencilView(mResource, &depthStencilViewDesc, &mDSV);
			AssertExpr(hr == S_OK);

			if (params.bIsShaderResource)
			{
				D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
				ZeroMemory(&srvDesc, sizeof(srvDesc));
				srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MostDetailedMip = params.MostDetailedMip;
				srvDesc.Texture2D.MipLevels = (params.MipLevels == 0) ? -1 : params.MipLevels;

				hr = mDevice->GetHardwareDevice().CreateShaderResourceView(mResource, &srvDesc, &mSRV);
				AssertExpr(hr == S_OK);
			}
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

	void DX11TextureBuffer2D::Release()
	{
		AssertNotNull(mResource);
		AssertNotNull(mSRV);
		AssertNotNull(mSamplerState);
		
		// #TODO
		// The if (!bIsDepthTexture)'s in this function are a result of an infrastructure
		// that is essentially DX11TextureBuffer2D. Because we don't need a RTV
		// for a depth texture, we won't have anything allocated here. This should be 
		// handled by subclassing different types of texture buffer uses (or something)
		if (!bIsDepthTexture)
		{
			AssertNotNull(mRTV);
		}

		mResource->Release();
		mSRV->Release();
		mSamplerState->Release();

		if (!bIsDepthTexture)
		{
			mRTV->Release();
		}
	}

	void DX11TextureBuffer2D::SetIsRenderTarget()
	{
		bIsRenderTarget = true;
	}

	void DX11TextureBuffer2D::SetDevice(DX11Device* device)
	{
		AssertNotNull(device);
		mDevice = device;
	}

	ID3D11ShaderResourceView& DX11TextureBuffer2D::GetResourceView()
	{
		AssertNotNull(mSRV);
		return *mSRV;
	}

	ID3D11RenderTargetView& DX11TextureBuffer2D::GetTargetView()
	{
		AssertNotNull(mRTV);
		return *mRTV;
	}

	ID3D11DepthStencilView& DX11TextureBuffer2D::GetDepthView()
	{
		AssertNotNull(mDSV);
		return *mDSV;
	}

	ID3D11SamplerState& DX11TextureBuffer2D::GetSamplerState()
	{
		AssertNotNull(mSamplerState);
		return *mSamplerState;
	}

	ID3D11Texture2D* DX11TextureBuffer2D::GetHWResource()
	{
		return mResource;
	}

}