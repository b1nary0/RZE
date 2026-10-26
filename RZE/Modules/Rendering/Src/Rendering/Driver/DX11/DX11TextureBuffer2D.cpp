#include <Rendering/Driver/DX11/DX11TextureBuffer2D.h>

#include <Rendering/Driver/DX11/DX11Device.h>

#include <Rendering/Driver/DX11/DX11.h>

#include <Utils/DebugUtils/Debug.h>

namespace Rendering
{
	namespace
	{
		DXGI_FORMAT ConvertToDXGIFormat(ETextureFormat format)
		{
			switch (format)
			{
			case ETextureFormat::RGBA8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
			case ETextureFormat::RGBA16_FLOAT: return DXGI_FORMAT_R16G16B16A16_FLOAT;
			case ETextureFormat::RG16_FLOAT: return DXGI_FORMAT_R16G16_FLOAT;
			case ETextureFormat::R32_UINT: return DXGI_FORMAT_R32_UINT;
			case ETextureFormat::RG32_UINT: return DXGI_FORMAT_R32G32_UINT;
			}

			AssertFalse();
			return DXGI_FORMAT_UNKNOWN;
		}

		// Releases a D3D object, if there is one, and forgets it
		template <typename TObject>
		void ReleaseCOM(TObject*& object)
		{
			if (object != nullptr)
			{
				object->Release();
				object = nullptr;
			}
		}

		U32 GetArraySize(const GFXTextureBufferParams& params)
		{
			return params.ArraySize > 1 ? params.ArraySize : 1;
		}

		// Reads the whole texture, as the kind of texture the params make
		D3D11_SHADER_RESOURCE_VIEW_DESC MakeResourceViewDesc(DXGI_FORMAT format, const GFXTextureBufferParams& params)
		{
			const U32 arraySize = GetArraySize(params);
			const UINT mipLevels = (params.MipLevels == 0) ? -1 : params.MipLevels;

			D3D11_SHADER_RESOURCE_VIEW_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.Format = format;

			if (params.bIsCubeMap && arraySize > 6)
			{
				desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
				desc.TextureCubeArray.MostDetailedMip = params.MostDetailedMip;
				desc.TextureCubeArray.MipLevels = mipLevels;
				desc.TextureCubeArray.First2DArrayFace = 0;
				desc.TextureCubeArray.NumCubes = arraySize / 6;
			}
			else if (params.bIsCubeMap)
			{
				desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
				desc.TextureCube.MostDetailedMip = params.MostDetailedMip;
				desc.TextureCube.MipLevels = mipLevels;
			}
			else if (arraySize > 1)
			{
				desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
				desc.Texture2DArray.MostDetailedMip = params.MostDetailedMip;
				desc.Texture2DArray.MipLevels = mipLevels;
				desc.Texture2DArray.FirstArraySlice = 0;
				desc.Texture2DArray.ArraySize = arraySize;
			}
			else
			{
				desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				desc.Texture2D.MostDetailedMip = params.MostDetailedMip;
				desc.Texture2D.MipLevels = mipLevels;
			}
			return desc;
		}

		// Writes sliceCount slices from firstSlice; a single texture (arraySize 1) has the plain 2D view
		D3D11_DEPTH_STENCIL_VIEW_DESC MakeDepthViewDesc(U32 arraySize, U32 firstSlice, U32 sliceCount)
		{
			D3D11_DEPTH_STENCIL_VIEW_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

			if (arraySize > 1)
			{
				desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
				desc.Texture2DArray.MipSlice = 0;
				desc.Texture2DArray.FirstArraySlice = firstSlice;
				desc.Texture2DArray.ArraySize = sliceCount;
			}
			else
			{
				desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
				desc.Texture2D.MipSlice = 0;
			}
			return desc;
		}

		// As MakeDepthViewDesc, for colour
		D3D11_RENDER_TARGET_VIEW_DESC MakeTargetViewDesc(DXGI_FORMAT format, U32 arraySize)
		{
			D3D11_RENDER_TARGET_VIEW_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.Format = format;

			if (arraySize > 1)
			{
				desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
				desc.Texture2DArray.MipSlice = 0;
				desc.Texture2DArray.FirstArraySlice = 0;
				desc.Texture2DArray.ArraySize = arraySize;
			}
			else
			{
				desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
				desc.Texture2D.MipSlice = 0;
			}
			return desc;
		}
	}

	DX11TextureBuffer2D::~DX11TextureBuffer2D()
	{
		Release();
	}

	void DX11TextureBuffer2D::Allocate(const void* data, const GFXTextureBufferParams& params)
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
		m_device->GetHardwareDevice().CreateSamplerState(&sampDesc, &m_samplerState);

		const U32 arraySize = GetArraySize(params);
		AssertMsg(!params.bIsCubeMap || arraySize % 6 == 0, "A cube map has 6 slices per cube");
		AssertMsg(arraySize == 1 || params.SampleCount <= 1, "Texture arrays can't be multisampled");

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
			textureDesc.ArraySize = arraySize;
			textureDesc.Format = (params.bIsShaderResource) ? DXGI_FORMAT_R24G8_TYPELESS : DXGI_FORMAT_D24_UNORM_S8_UINT;
			textureDesc.SampleDesc.Count = 1;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Usage = D3D11_USAGE_DEFAULT;
			textureDesc.BindFlags = (params.bIsShaderResource) ? D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE : D3D11_BIND_DEPTH_STENCIL;
			textureDesc.CPUAccessFlags = 0;
			textureDesc.MiscFlags = params.bIsCubeMap ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;

			HRESULT hr = m_device->GetHardwareDevice().CreateTexture2D(&textureDesc, NULL, &m_resource);
			AssertExpr(hr == S_OK);

			// Every slice, for clearing them all at once
			const D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = MakeDepthViewDesc(arraySize, 0, arraySize);
			hr = m_device->GetHardwareDevice().CreateDepthStencilView(m_resource, &depthStencilViewDesc, &m_DSV);
			AssertExpr(hr == S_OK);

			// One slice each, for rendering into them one at a time (e.g. a cube's faces)
			if (arraySize > 1)
			{
				m_sliceDSVs.resize(arraySize, nullptr);
				for (U32 slice = 0; slice < arraySize; ++slice)
				{
					const D3D11_DEPTH_STENCIL_VIEW_DESC sliceViewDesc = MakeDepthViewDesc(arraySize, slice, 1);
					hr = m_device->GetHardwareDevice().CreateDepthStencilView(m_resource, &sliceViewDesc, &m_sliceDSVs[slice]);
					AssertExpr(hr == S_OK);
				}
			}

			if (params.bIsShaderResource)
			{
				const D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = MakeResourceViewDesc(DXGI_FORMAT_R24_UNORM_X8_TYPELESS, params);
				hr = m_device->GetHardwareDevice().CreateShaderResourceView(m_resource, &srvDesc, &m_SRV);
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
			textureDesc.ArraySize = arraySize;
			textureDesc.Format = ConvertToDXGIFormat(params.Format);
			textureDesc.SampleDesc.Count = params.SampleCount;
			textureDesc.SampleDesc.Quality = params.SampleQuality;
			textureDesc.Usage = D3D11_USAGE_DEFAULT;

			UINT bindFlags = 0;
			if (params.bIsShaderResource) bindFlags = (bindFlags | D3D11_BIND_SHADER_RESOURCE);
			if (params.bIsRenderTarget) bindFlags = (bindFlags | D3D11_BIND_RENDER_TARGET);

			// Integer formats don't support mip generation, and creating one with the flag fails
			const bool canGenerateMips = params.Format != ETextureFormat::R32_UINT && params.Format != ETextureFormat::RG32_UINT;

			textureDesc.BindFlags = bindFlags;
			textureDesc.CPUAccessFlags = 0;
			textureDesc.MiscFlags = (canGenerateMips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0)
				| (params.bIsCubeMap ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0);

			const D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = MakeResourceViewDesc(textureDesc.Format, params);
			const D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = MakeTargetViewDesc(textureDesc.Format, arraySize);

			HRESULT hr = m_device->GetHardwareDevice().CreateTexture2D(&textureDesc, NULL, &m_resource);
			AssertExpr(hr == S_OK);

			if (data != nullptr)
			{
				// Only the first slice would be filled
				AssertMsg(arraySize == 1, "Texture arrays can't be made with texels");
				// Row pitch below assumes 4 bytes per texel
				AssertExpr(params.Format == ETextureFormat::RGBA8_UNORM);
				U32 rowPitch = (32 * params.Width) / 8;
				m_device->GetDeviceContext().UpdateSubresource(m_resource, 0, NULL, data, rowPitch, 0);
			}

			hr = m_device->GetHardwareDevice().CreateRenderTargetView(m_resource, &rtvDesc, &m_RTV);
			AssertExpr(hr == S_OK);
			hr = m_device->GetHardwareDevice().CreateShaderResourceView(m_resource, &srvDesc, &m_SRV);
			AssertExpr(hr == S_OK);

			if (canGenerateMips)
			{
				m_device->GetDeviceContext().GenerateMips(m_SRV);
			}
		}
	}

	void DX11TextureBuffer2D::Release()
	{
		// Only what Allocate made: a depth texture has a depth view and no render target view, and a texture dropped
		// before the render thread made it has nothing. Each view holds a reference to the texture, so all of them have
		// to go for it to be freed.
		for (ID3D11DepthStencilView*& sliceDSV : m_sliceDSVs)
		{
			ReleaseCOM(sliceDSV);
		}
		m_sliceDSVs.clear();
		ReleaseCOM(m_DSV);
		ReleaseCOM(m_RTV);
		ReleaseCOM(m_SRV);
		ReleaseCOM(m_samplerState);
		ReleaseCOM(m_resource);
	}

	void DX11TextureBuffer2D::SetActive(U32 textureSlot)
	{
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.PSSetShaderResources(textureSlot, 1, &m_SRV);
		deviceContext.PSSetSamplers(0, 1, &m_samplerState);
	}

	void* DX11TextureBuffer2D::GetTextureData()
	{
		return &GetResourceView();
	}

	bool DX11TextureBuffer2D::IsValid()
	{
		return m_device != nullptr && m_SRV != nullptr;
	}

	void DX11TextureBuffer2D::SetDevice(DX11Device* device)
	{
		AssertNotNull(device);
		m_device = device;
	}

	ID3D11ShaderResourceView& DX11TextureBuffer2D::GetResourceView()
	{
		AssertNotNull(m_SRV);
		return *m_SRV;
	}

	ID3D11RenderTargetView& DX11TextureBuffer2D::GetTargetView()
	{
		AssertNotNull(m_RTV);
		return *m_RTV;
	}

	ID3D11DepthStencilView& DX11TextureBuffer2D::GetDepthView()
	{
		AssertNotNull(m_DSV);
		return *m_DSV;
	}

	ID3D11DepthStencilView& DX11TextureBuffer2D::GetDepthSliceView(U32 slice)
	{
		// A single texture's one slice is the whole texture
		if (m_sliceDSVs.empty())
		{
			AssertExpr(slice == 0);
			return GetDepthView();
		}

		AssertExpr(slice < m_sliceDSVs.size());
		AssertNotNull(m_sliceDSVs[slice]);
		return *m_sliceDSVs[slice];
	}

	ID3D11SamplerState& DX11TextureBuffer2D::GetSamplerState()
	{
		AssertNotNull(m_samplerState);
		return *m_samplerState;
	}

	ID3D11Texture2D* DX11TextureBuffer2D::GetHWResource()
	{
		return m_resource;
	}

}