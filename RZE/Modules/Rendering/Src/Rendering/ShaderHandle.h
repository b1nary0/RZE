#pragma once

#include <Rendering/Driver/ShaderTypes.h>

#include <memory>

namespace Rendering
{
	class VertexShaderHandle
	{
		// @todo remove this after Renderer only issues commands
		friend class Renderer;
		// @todo come back to this. right now the only places that should be able to call reset
		// on the smart_ptr is a command. that way we can guarantee you're going through the right places
		// to handle it. but not convinced at this approach at all. its ugly, but works
		friend struct RenderCommand_ReleaseVertexShader;

	public:
		VertexShaderHandle() = default;
		virtual ~VertexShaderHandle() = default;

	public:
		bool IsValid() { return m_shader != nullptr; }

	private:
		VertexShaderHandle(const std::shared_ptr<IVertexShader>& shader)
			: m_shader(shader) {}

		std::shared_ptr<IVertexShader> m_shader;
	};

	class PixelShaderHandle
	{
		friend class Renderer;
		friend struct RenderCommand_ReleasePixelShader;

	public:
		PixelShaderHandle() = default;
		virtual ~PixelShaderHandle() = default;

		bool IsValid() { return m_shader != nullptr; }

	private:
		PixelShaderHandle(const std::shared_ptr<IPixelShader>& shader)
			: m_shader(shader) {}

		std::shared_ptr<IPixelShader> m_shader;
	};
}