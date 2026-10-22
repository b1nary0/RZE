#include <Rendering/Driver/DX11/DX11ShaderTypes.h>

#include <Rendering/Driver/DX11/DX11.h>
#include <Rendering/Driver/DX11/DX11Device.h>
#include <Rendering/Driver/DX11/DX11TypeConversion.h>

#include <Utils/Conversions.h>
#include <Utils/DebugUtils/Debug.h>

#include "Rendering/Driver/ShaderTypes.h"

#include <string_view>

namespace
{
	// Logs what compiling a shader reported (D3DCompile's error blob): errors if it failed, otherwise warnings
	void LogCompileMessages(HRESULT hr, ID3D10Blob* messages, const Filepath& filePath)
	{
		if (messages == nullptr)
		{
			return;
		}

		// Without its trailing newline, since the log adds its own
		std::string_view text = static_cast<const char*>(messages->GetBufferPointer());
		while (!text.empty() && (text.back() == '\n' || text.back() == '\r'))
		{
			text.remove_suffix(1);
		}

		if (FAILED(hr))
		{
			RZE_LOG_ERROR("Couldn't compile shader [" << filePath.GetRelativePath() << "]:\n" << text);
		}
		else
		{
			RZE_LOG_WARNING("Shader [" << filePath.GetRelativePath() << "] compiled with warnings:\n" << text);
		}
	}
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

	void DX11VertexShader::Create(const Filepath& filePath, const ShaderInputLayout& inputLayout)
	{
		HRESULT hr;
		ID3D10Blob* error = nullptr;;
		ID3D10Blob* vsBlob = nullptr;
		
		hr = D3DCompileFromFile(Conversions::StringToWString(filePath.GetAbsolutePath()).c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_0", 0, 0, &vsBlob, &error);
		LogCompileMessages(hr, error, filePath);
		
		hr = m_graphicsDevice->GetHardwareDevice().CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_hwShader);
		
		std::unique_ptr<D3D11_INPUT_ELEMENT_DESC> inputLayoutStructure = std::unique_ptr<D3D11_INPUT_ELEMENT_DESC>(new D3D11_INPUT_ELEMENT_DESC[inputLayout.GetDataCount()]);

		for (size_t i = 0; i < inputLayout.GetDataCount(); ++i)
		{
			const ShaderInputLayout::ShaderInputLayoutData& layoutData = inputLayout[i];
			inputLayoutStructure.get()[i] = {layoutData.Name.c_str(), 0, ConvertToDXGIFormat(layoutData.DataFormat), 0, static_cast<U32>(layoutData.Offset), ConvertToD3D11InputClassification(layoutData.DataClassification), 0};
		}

		UINT numLayoutElements = static_cast<UINT>(inputLayout.GetDataCount());

		hr = m_graphicsDevice->GetHardwareDevice().CreateInputLayout(inputLayoutStructure.get(), numLayoutElements, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_hwInputLayout);
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

	void DX11VertexShader::SetInputLayout()
	{
		ID3D11DeviceContext& deviceContext = m_graphicsDevice->GetDeviceContext();
		deviceContext.IASetInputLayout(m_hwInputLayout);
	}

	DX11PixelShader::DX11PixelShader()
	{
	}

	DX11PixelShader::~DX11PixelShader()
	{
		Release();
	}

	void DX11PixelShader::Create(const Filepath& filePath)
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
		LogCompileMessages(hr, error, filePath);
		
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
		deviceContext.PSSetShader(m_hwShader, nullptr, 0);
	}

	void DX11PixelShader::SetDevice(DX11Device* graphicsDevice)
	{
		AssertNotNull(graphicsDevice);
		m_graphicsDevice = graphicsDevice;
	}
}
