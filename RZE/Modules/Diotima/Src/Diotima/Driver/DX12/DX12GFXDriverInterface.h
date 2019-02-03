#pragma once

#include <Diotima/Driver/GFXDriverInterface.h>

namespace Diotima
{
	class DX12GFXDriverInterface : public IGFXDriverInterface
	{
	public:
		DX12GFXDriverInterface() = default;
		virtual ~DX12GFXDriverInterface() = default;

	public:
		virtual void Initialize() final override;
	};
}