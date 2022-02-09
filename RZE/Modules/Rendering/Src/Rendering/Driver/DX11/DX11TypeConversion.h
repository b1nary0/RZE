#pragma once

#include <Rendering/Driver/DX11/DX11.h>
#include <Rendering/Driver/TypeDefines.h>

inline DXGI_FORMAT ConvertToDXGIFormat(Rendering::EDataFormat dataFormat)
{
	switch (dataFormat)
	{
		case Rendering::EDataFormat::R32G32_FLOAT: return DXGI_FORMAT_R32G32_FLOAT;
		case Rendering::EDataFormat::R32G32B32_FLOAT: return DXGI_FORMAT_R32G32B32_FLOAT;
	}

	return DXGI_FORMAT_UNKNOWN;
}

inline D3D11_INPUT_CLASSIFICATION ConvertToD3D11InputClassification(Rendering::EDataClassification inputClassification)
{
	switch (inputClassification)
	{
	case Rendering::EDataClassification::PER_VERTEX: return D3D11_INPUT_PER_VERTEX_DATA;
	case Rendering::EDataClassification::PER_INSTANCE: return D3D11_INPUT_PER_INSTANCE_DATA;
	}

	return D3D11_INPUT_PER_VERTEX_DATA;
}
