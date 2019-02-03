#pragma once


//
// GFXDriverInterface
//
// Each driver may have their individual ecosystem setups outside of devices, etc. 
// For example: D3D has the debug layer which needs to be initialized. This class
// aims to help wrap that for each driver API to avoid bespoke setup in the renderer.
// The driver interface will also act as a factory of sorts to create the device etc.
// Ideally this will also help obfuscate the header includes.
namespace Diotima
{
	class IGFXDriverInterface
	{
	public:
		IGFXDriverInterface() = default;
		virtual ~IGFXDriverInterface() = default;

	public:
		virtual void Initialize() = 0;
	};
}