#pragma once

#include <Utils/PrimitiveDefs.h>

namespace Rendering
{
	// Texels for Renderer::CreateTextureBuffer2D to upload, which owns and frees them. The upload command keeps them until
	// the render thread has made the texture, so whoever loaded them can let go as soon as it asks for the texture. They're
	// freed with the function they come with, so they can come from any allocator (e.g. stbi_image_free).
	class TexelBuffer
	{
	public:
		using FreeFunction = void (*)(void*);

	public:
		TexelBuffer() = default;
		TexelBuffer(U8* texels, FreeFunction freeTexels);
		~TexelBuffer();

		TexelBuffer(const TexelBuffer&) = delete;
		TexelBuffer& operator=(const TexelBuffer&) = delete;

		TexelBuffer(TexelBuffer&& other) noexcept;
		TexelBuffer& operator=(TexelBuffer&& other) noexcept;

	public:
		const U8* GetTexels() const { return m_texels; }
		bool IsEmpty() const { return m_texels == nullptr; }

		// Frees the texels now, leaving it empty
		void Reset();

	private:
		U8* m_texels = nullptr;
		FreeFunction m_freeTexels = nullptr;
	};
}
