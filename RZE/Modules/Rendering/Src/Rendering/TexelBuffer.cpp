#include <Rendering/TexelBuffer.h>

#include <Utils/DebugUtils/Debug.h>

namespace Rendering
{
	TexelBuffer::TexelBuffer(U8* texels, FreeFunction freeTexels)
		: m_texels(texels)
		, m_freeTexels(freeTexels)
	{
		AssertExpr(texels == nullptr || freeTexels != nullptr);
	}

	TexelBuffer::~TexelBuffer()
	{
		Reset();
	}

	TexelBuffer::TexelBuffer(TexelBuffer&& other) noexcept
		: m_texels(other.m_texels)
		, m_freeTexels(other.m_freeTexels)
	{
		other.m_texels = nullptr;
		other.m_freeTexels = nullptr;
	}

	TexelBuffer& TexelBuffer::operator=(TexelBuffer&& other) noexcept
	{
		if (this != &other)
		{
			Reset();

			m_texels = other.m_texels;
			m_freeTexels = other.m_freeTexels;
			other.m_texels = nullptr;
			other.m_freeTexels = nullptr;
		}
		return *this;
	}

	void TexelBuffer::Reset()
	{
		if (m_texels != nullptr)
		{
			m_freeTexels(m_texels);
			m_texels = nullptr;
		}
		m_freeTexels = nullptr;
	}
}
