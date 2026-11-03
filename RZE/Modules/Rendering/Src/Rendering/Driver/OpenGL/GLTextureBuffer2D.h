#pragma once

#include <Rendering/Driver/GFXBuffer.h>

namespace Rendering
{
	class GLDevice;

	// A texture, of whatever kind its params make: 2D, a 2D array, a cube, or a cube array
	class GLTextureBuffer2D : public ITextureBuffer2D
	{
	public:
		// The GL format of an ETextureFormat (or of a depth texture), for making, clearing, copying and reading it
		struct Format
		{
			U32 InternalFormat = 0;
			U32 PixelFormat = 0;
			U32 PixelType = 0;
			U32 BytesPerTexel = 0;
			bool IsInteger = false;
			bool IsDepth = false;
		};

		explicit GLTextureBuffer2D(GLDevice& device);
		~GLTextureBuffer2D() override;

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
		U32 GetTexture() const { return m_texture; }
		U32 GetTarget() const { return m_target; }
		const Format& GetFormat() const { return m_format; }
		const GFXTextureBufferParams& GetParams() const { return m_params; }
		U32 GetLayerCount() const { return m_layerCount; }
		U32 GetMipCount() const { return m_mipCount; }
		// What a compute shader writes it through: an array, cube maps among them, as D3D's UAVs are each a
		// RWTexture2DArray of its slices. A cube map's own type wouldn't match the shader's image2DArray, so it gets a
		// 2D array view of itself, made the first time it's asked for. Render thread only.
		U32 GetImageTexture();

	private:
		GLDevice& m_device;
		GFXTextureBufferParams m_params = {};
		Format m_format;
		U32 m_texture = 0;
		// A cube map's 2D array view (GetImageTexture); 0 until asked for
		U32 m_arrayView = 0;
		U32 m_target = 0;
		// Slices, cube faces among them: 6 per cube
		U32 m_layerCount = 1;
		U32 m_mipCount = 1;
	};
}
