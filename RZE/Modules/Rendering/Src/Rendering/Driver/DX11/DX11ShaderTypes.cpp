#include <Rendering/Driver/DX11/DX11ShaderTypes.h>

#include <Rendering/Driver/DX11/DX11.h>
#include <Rendering/Driver/DX11/DX11Device.h>

#include <Utils/Conversions.h>

namespace
{
	auto errorCB = [](ID3D10Blob* error)
	{
		if (error != nullptr)
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
	};
}

namespace Rendering
{
	DX11VertexShader::DX11VertexShader()
	{

	}

	DX11VertexShader::~DX11VertexShader()
	{
		Release();
	}

	void DX11VertexShader::Create(const FilePath& filePath)
	{
		HRESULT hr;
		ID3D10Blob* error = nullptr;;
		ID3D10Blob* vsBlob = nullptr;
		
		hr = D3DCompileFromFile(Conversions::StringToWString(filePath.GetAbsolutePath()).c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_0", 0, 0, &vsBlob, &error);
		errorCB(error);
		
		hr = m_graphicsDevice->GetHardwareDevice().CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_hwShader);

		// @TODO Move this
		// Need to drive this from BufferLayout class instead - just putting this here to get stuff working.
		// Not really feeling hardware abstraction at all. Might just leave it at D3D...
		D3D11_INPUT_ELEMENT_DESC k_vertLayout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		UINT numLayoutElements = ARRAYSIZE(k_vertLayout);

		hr = m_graphicsDevice->GetHardwareDevice().CreateInputLayout(k_vertLayout, numLayoutElements, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_hwInputLayout);
	}

	void DX11VertexShader::Release()
	{
		AssertNotNull(m_hwShader);
		m_hwShader->Release();
	}

	void DX11VertexShader::SetActive()
	{
		ID3D11DeviceContext& deviceContext = m_graphicsDevice->GetDeviceContext();
		deviceContext.VSSetShader(m_hwShader, 0, 0);
	}

	void DX11VertexShader::SetDevice(DX11Device* graphicsDevice)
	{
		AssertNotNull(graphicsDevice);
		m_graphicsDevice = graphicsDevice;
	}

	DX11PixelShader::DX11PixelShader()
	{
	}

	DX11PixelShader::~DX11PixelShader()
	{
		Release();
	}

	void DX11PixelShader::Create(const FilePath& filePath)
	{
		ID3D10Blob* psBlob = nullptr;
		ID3D10Blob* error = nullptr;

		HRESULT hr;
		hr = D3DCompileFromFile(
			Conversions::StringToWString(filePath.GetAbsolutePath()).c_str(),
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"PSMain", "ps_5_0",
			0, 0,
			&psBlob, &error);
		errorCB(error);
		
		hr = m_graphicsDevice->GetHardwareDevice().CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_hwShader);
	}

	void DX11PixelShader::Release()
	{
		AssertNotNull(m_hwShader);
		m_hwShader->Release();
	}

	void DX11PixelShader::SetActive()
	{
		ID3D11DeviceContext& deviceContext = m_graphicsDevice->GetDeviceContext();
		deviceContext.PSSetShader(m_hwShader, 0, 0);
	}

	void DX11PixelShader::SetDevice(DX11Device* graphicsDevice)
	{
		AssertNotNull(graphicsDevice);
		m_graphicsDevice = graphicsDevice;
	}
}
