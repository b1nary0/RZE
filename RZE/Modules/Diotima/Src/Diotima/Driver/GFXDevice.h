#pragma once

namespace Diotima
{
	class IGFXVertexBuffer;

	class IGFXDevice
	{
	public:
		IGFXDevice() = default;
		virtual ~IGFXDevice() = default;

	public:
		virtual void Initialize() = 0;
		virtual void SetWindow(void* windowHandle) = 0;
		virtual void Shutdown() = 0;

		virtual IGFXVertexBuffer* CreateBuffer(const std::vector<float>& data) = 0;
	};
}