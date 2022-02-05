#pragma once

namespace Rendering
{
	class IGFXDevice
	{
	public:
		IGFXDevice() = default;
		virtual ~IGFXDevice() = default;

	public:
		virtual void Initialize() = 0;
		virtual void Present() = 0;
		virtual void SetWindow(void* windowHandle) = 0;
		virtual void Shutdown() = 0;
	};
}