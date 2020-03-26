#include <Diotima/Driver/DX11/DX11GFXTextureBuffer2D.h>

#include <Diotima/Driver/DX11/DX11GFXDevice.h>

#include <Diotima/Driver/DX11/DX11.h>

#include <Utils/DebugUtils/Debug.h>

namespace Diotima
{

	void DX11GFXTextureBuffer2D::Allocate(void* data, U32 width, U32 height)
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

		D3D11_TEXTURE2D_DESC textureDesc;
		ZeroMemory(&textureDesc, sizeof(textureDesc));
		textureDesc.Height = height;
		textureDesc.Width = width;
		textureDesc.MipLevels = 0;
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		textureDesc.CPUAccessFlags = 0;
		textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

		HRESULT hr = mDevice->GetHardwareDevice().CreateTexture2D(&textureDesc, NULL, &mResource);
		AssertExpr(hr == S_OK);

		U32 rowPitch = (32 * width) / 8;
		mDevice->GetDeviceContext().UpdateSubresource(mResource, 0, NULL, data, rowPitch, 0);

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = -1;

		hr = mDevice->GetHardwareDevice().CreateShaderResourceView(mResource, &srvDesc, &mResourceView);
		AssertExpr(hr == S_OK);

		mDevice->GetDeviceContext().GenerateMips(mResourceView);
	}

	void DX11GFXTextureBuffer2D::SetDevice(DX11GFXDevice* device)
	{
		AssertNotNull(device);
		mDevice = device;
	}

	ID3D11ShaderResourceView& DX11GFXTextureBuffer2D::GetResourceView()
	{
		AssertNotNull(mResourceView);
		return *mResourceView;
	}

	ID3D11SamplerState& DX11GFXTextureBuffer2D::GetSamplerState()
	{
		AssertNotNull(mSamplerState);
		return *mSamplerState;
	}

}