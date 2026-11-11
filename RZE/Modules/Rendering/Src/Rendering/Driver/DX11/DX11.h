#pragma once

#include <d3d11.h>
#include <dxgi1_5.h>
#include <d3d10.h>
#include <d3dcompiler.h>

#include <Utils/PrimitiveDefs.h>

namespace Rendering
{
	// The size of one texel of a format the backend makes or reads back (DX11Device.cpp)
	U32 GetBytesPerTexel(DXGI_FORMAT format);
}