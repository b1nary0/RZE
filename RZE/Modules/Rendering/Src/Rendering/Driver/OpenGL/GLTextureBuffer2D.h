#pragma once

#include <Rendering/Driver/GFXBuffer.h>

namespace Rendering
{
	class GLDevice;

	class GLTextureBuffer2D : public ITextureBuffer2D
	{
	public:
		explicit GLTextureBuffer2D(GLDevice& device);

	// ITextureBuffer2D interface
	public:
		void Allocate(const void* data, const GFXTextureBufferParams& params) override;
		void Release() override;
		void SetActive(U32 textureSlot) override;
		void SetActiveCS(U32 textureSlot) override;
		// The GL texture's name, as ImGui's OpenGL backend takes it
		void* GetTextureData() override;
		bool IsValid() override;

	public:
		// What it was allocated with
		const GFXTextureBufferParams& GetParams() const { return m_params; }

	private:
		[[maybe_unused]] GLDevice& m_device;
		GFXTextureBufferParams m_params = {};
	};
}
