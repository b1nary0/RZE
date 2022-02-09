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