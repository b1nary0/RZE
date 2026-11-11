#include <Rendering/Driver/OpenGL/GLTextureBuffer2D.h>

#include <Rendering/Driver/OpenGL/GLCommon.h>
#include <Rendering/Driver/OpenGL/GLDevice.h>
#include <Rendering/Driver/OpenGL/GLShaderTranslator.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Rendering
{
	namespace
	{
		GLTextureBuffer2D::Format ToGLFormat(const GFXTextureBufferParams& params)
		{
			GLTextureBuffer2D::Format format;
			if (params.bIsDepthTexture)
			{
				// D24S8, as the D3D depth textures are
				format.InternalFormat = GL_DEPTH24_STENCIL8;
				format.PixelFormat = GL_DEPTH_STENCIL;
				format.PixelType = GL_UNSIGNED_INT_24_8;
				format.BytesPerTexel = 4;
				format.IsDepth = true;
				return format;
			}

			switch (params.Format)
			{
			case ETextureFormat::RGBA8_UNORM: format = { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, 4, false, false }; break;
			case ETextureFormat::RGBA16_FLOAT: format = { GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT, 8, false, false }; break;
			case ETextureFormat::RG16_FLOAT: format = { GL_RG16F, GL_RG, GL_HALF_FLOAT, 4, false, false }; break;
			case ETextureFormat::R32_UINT: format = { GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, 4, true, false }; break;
			case ETextureFormat::RG32_UINT: format = { GL_RG32UI, GL_RG_INTEGER, GL_UNSIGNED_INT, 8, true, false }; break;
			case ETextureFormat::R8_UNORM: format = { GL_R8, GL_RED, GL_UNSIGNED_BYTE, 1, false, false }; break;
			case ETextureFormat::R16_FLOAT: format = { GL_R16F, GL_RED, GL_HALF_FLOAT, 2, false, false }; break;
			case ETextureFormat::R32_FLOAT: format = { GL_R32F, GL_RED, GL_FLOAT, 4, false, false }; break;
			case ETextureFormat::RG8_UNORM: format = { GL_RG8, GL_RG, GL_UNSIGNED_BYTE, 2, false, false }; break;
			}
			return format;
		}

		// D3D's whole chain for MipLevels 0: down to 1x1
		U32 GetFullMipCount(U32 width, U32 height)
		{
			return 1 + static_cast<U32>(std::floor(std::log2(static_cast<double>(std::max(width, height)))));
		}
	}

	GLTextureBuffer2D::GLTextureBuffer2D(GLDevice& device)
		: m_device(device)
	{
	}

	GLTextureBuffer2D::~GLTextureBuffer2D()
	{
		Release();
	}

	void GLTextureBuffer2D::Allocate(const void* data, const GFXTextureBufferParams& params)
	{
		m_params = params;
		m_format = ToGLFormat(params);
		m_layerCount = params.ArraySize > 1 ? params.ArraySize : 1;
		AssertMsg(!params.bIsCubeMap || m_layerCount % 6 == 0, "A cube map has 6 slices per cube");
		AssertMsg(params.SampleCount <= 1, "Multisampled textures aren't supported by the OpenGL renderer");

		// Depth textures have one mip, as on D3D
		m_mipCount = m_format.IsDepth ? 1 : (params.MipLevels == 0 ? GetFullMipCount(params.Width, params.Height) : params.MipLevels);

		if (params.bIsCubeMap)
		{
			m_target = m_layerCount > 6 ? GL_TEXTURE_CUBE_MAP_ARRAY : GL_TEXTURE_CUBE_MAP;
		}
		else
		{
			m_target = m_layerCount > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
		}

		glCreateTextures(m_target, 1, &m_texture);
		const GLsizei width = static_cast<GLsizei>(params.Width);
		const GLsizei height = static_cast<GLsizei>(params.Height);
		if (m_target == GL_TEXTURE_2D || m_target == GL_TEXTURE_CUBE_MAP)
		{
			glTextureStorage2D(m_texture, static_cast<GLsizei>(m_mipCount), m_format.InternalFormat, width, height);
		}
		else
		{
			// A cube array's depth is its number of faces
			glTextureStorage3D(m_texture, static_cast<GLsizei>(m_mipCount), m_format.InternalFormat, width, height, static_cast<GLsizei>(m_layerCount));
		}

		// Its own sampling, which applies when no sampler object is bound (ImGui, and reads with Load): what D3D's
		// default samples a texture with, linear and wrapping. GL won't filter integer textures, so those take the
		// nearest texel; and it compares depth only when a shadow sampler asks.
		const bool isFiltered = !m_format.IsInteger;
		glTextureParameteri(m_texture, GL_TEXTURE_MIN_FILTER, isFiltered ? (m_mipCount > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR) : GL_NEAREST);
		glTextureParameteri(m_texture, GL_TEXTURE_MAG_FILTER, isFiltered ? GL_LINEAR : GL_NEAREST);
		glTextureParameteri(m_texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(m_texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTextureParameteri(m_texture, GL_TEXTURE_WRAP_R, GL_REPEAT);
		glTextureParameteri(m_texture, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(m_mipCount) - 1);

		if (data != nullptr)
		{
			// Only the first slice would be filled
			AssertMsg(m_layerCount == 1, "Texture arrays can't be made with texels");
			// Tightly packed rows, which GLDevice sets GL_UNPACK_ALIGNMENT for
			glTextureSubImage2D(m_texture, 0, 0, 0, width, height, m_format.PixelFormat, m_format.PixelType, data);
		}

		// As the D3D one: render targets that can have mips get them now, from whatever their first mip holds
		const bool canGenerateMips = params.bIsRenderTarget && !m_format.IsInteger && !m_format.IsDepth;
		if (canGenerateMips && m_mipCount > 1)
		{
			glGenerateTextureMipmap(m_texture);
		}
	}

	U32 GLTextureBuffer2D::GetImageTexture()
	{
		if (m_target != GL_TEXTURE_CUBE_MAP && m_target != GL_TEXTURE_CUBE_MAP_ARRAY)
		{
			return m_texture;
		}

		if (m_arrayView == 0)
		{
			glGenTextures(1, &m_arrayView);
			glTextureView(m_arrayView, GL_TEXTURE_2D_ARRAY, m_texture, m_format.InternalFormat, 0, m_mipCount, 0, m_layerCount);
		}
		return m_arrayView;
	}

	void GLTextureBuffer2D::Release()
	{
		if (m_arrayView != 0)
		{
			m_device.DeleteLater(GLDevice::EObjectType::Texture, m_arrayView);
			m_arrayView = 0;
		}
		if (m_texture != 0)
		{
			m_device.DeleteLater(GLDevice::EObjectType::Texture, m_texture);
			m_texture = 0;
		}
	}

	void GLTextureBuffer2D::SetActive(U32 textureSlot)
	{
		m_device.SetTextureResource(GLShaderTranslator::EStage::Pixel, textureSlot, this);
	}

	void GLTextureBuffer2D::SetActiveCS(U32 textureSlot)
	{
		m_device.SetTextureResource(GLShaderTranslator::EStage::Compute, textureSlot, this);
	}

	void* GLTextureBuffer2D::GetTextureData()
	{
		return reinterpret_cast<void*>(static_cast<uintptr_t>(m_texture));
	}

	bool GLTextureBuffer2D::IsValid()
	{
		return m_texture != 0;
	}
}
