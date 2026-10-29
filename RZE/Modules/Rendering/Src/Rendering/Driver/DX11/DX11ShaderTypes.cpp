#include <Rendering/Driver/DX11/DX11ShaderTypes.h>

#include <Rendering/Driver/DX11/DX11.h>
#include <Rendering/Driver/DX11/DX11Device.h>
#include <Rendering/Driver/DX11/DX11TypeConversion.h>

#include <Utils/Conversions.h>
#include <Utils/DebugUtils/Debug.h>

#include "Rendering/Driver/ShaderTypes.h"

#include <string_view>
#include <vector>

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
		ID3D10Blob* vsBlob = nullptr;
		ID3D10Blob* error = nullptr;

		HRESULT hr = D3DCompileFromFile(
			Conversions::StringToWString(filePath.GetAbsolutePath()).c_str(),
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"VSMain", "vs_5_0",
			0, 0,
			&vsBlob, &error);
		LogCompileMessages(hr, error, filePath);
		if (error != nullptr)
		{
			error->Release();
		}

		if (FAILED(hr))
		{
			return;
		}

		hr = m_graphicsDevice->GetHardwareDevice().CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_hwShader);
		AssertExpr(hr == S_OK);

		// Shaders that make their own vertices (from SV_VertexID) take no vertex buffer, so no layout: D3D won't make an
		// empty one, and draws without one are what they want
		if (inputLayout.GetDataCount() == 0)
		{
			vsBlob->Release();
			return;
		}

		std::vector<D3D11_INPUT_ELEMENT_DESC> inputElements;
		inputElements.reserve(inputLayout.GetDataCount());
		for (size_t i = 0; i < inputLayout.GetDataCount(); ++i)
		{
			const ShaderInputLayout::ShaderInputLayoutData& layoutData = inputLayout[i];
			const bool isPerInstance = layoutData.DataClassification == EDataClassification::PER_INSTANCE;
			inputElements.push_back({ layoutData.Name.c_str(), 0, ConvertToDXGIFormat(layoutData.DataFormat), layoutData.InputSlot, static_cast<U32>(layoutData.Offset),
				ConvertToD3D11InputClassification(layoutData.DataClassification), isPerInstance ? 1u : 0u });
		}

		hr = m_graphicsDevice->GetHardwareDevice().CreateInputLayout(inputElements.data(), static_cast<UINT>(inputElements.size()), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_hwInputLayout);
		vsBlob->Release();

		// The shader's inputs don't match the layout the engine gives it, e.g. after editing them in the .hlsl
		if (FAILED(hr))
		{
			RZE_LOG_ERROR("Shader [" << filePath.GetRelativePath() << "] doesn't take the vertex layout it's given");
			Release();
		}
	}

	void DX11VertexShader::Release()
	{
		if (m_hwInputLayout != nullptr)
		{
			m_hwInputLayout->Release();
			m_hwInputLayout = nullptr;
		}
		if (m_hwShader != nullptr)
		{
			m_hwShader->Release();
			m_hwShader = nullptr;
		}
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

		HRESULT hr = D3DCompileFromFile(
			Conversions::StringToWString(filePath.GetAbsolutePath()).c_str(),
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"PSMain", "ps_5_0",
			0, 0,
			&psBlob, &error);
		LogCompileMessages(hr, error, filePath);
		if (error != nullptr)
		{
			error->Release();
		}

		if (FAILED(hr))
		{
			return;
		}

		hr = m_graphicsDevice->GetHardwareDevice().CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_hwShader);
		AssertExpr(hr == S_OK);
		psBlob->Release();
	}

	void DX11PixelShader::Release()
	{
		if (m_hwShader != nullptr)
		{
			m_hwShader->Release();
			m_hwShader = nullptr;
		}
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

	DX11ComputeShader::DX11ComputeShader()
	{
	}

	DX11ComputeShader::~DX11ComputeShader()
	{
		Release();
	}

	void DX11ComputeShader::Create(const Filepath& filePath)
	{
		ID3D10Blob* csBlob = nullptr;
		ID3D10Blob* error = nullptr;

		HRESULT hr = D3DCompileFromFile(
			Conversions::StringToWString(filePath.GetAbsolutePath()).c_str(),
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"CSMain", "cs_5_0",
			0, 0,
			&csBlob, &error);
		LogCompileMessages(hr, error, filePath);
		if (error != nullptr)
		{
			error->Release();
		}

		if (FAILED(hr))
		{
			return;
		}

		hr = m_graphicsDevice->GetHardwareDevice().CreateComputeShader(csBlob->GetBufferPointer(), csBlob->GetBufferSize(), nullptr, &m_hwShader);
		AssertExpr(hr == S_OK);
		csBlob->Release();
	}

	void DX11ComputeShader::Release()
	{
		if (m_hwShader != nullptr)
		{
			m_hwShader->Release();
			m_hwShader = nullptr;
		}
	}

	void DX11ComputeShader::SetActive()
	{
		ID3D11DeviceContext& deviceContext = m_graphicsDevice->GetDeviceContext();
		deviceContext.CSSetShader(m_hwShader, nullptr, 0);
	}

	void DX11ComputeShader::SetDevice(DX11Device* graphicsDevice)
	{
		AssertNotNull(graphicsDevice);
		m_graphicsDevice = graphicsDevice;
	}
}
