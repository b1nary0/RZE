#pragma once

#include <Rendering/Driver/ShaderTypes.h>

#include <memory>

namespace Rendering
{
	class VertexShaderHandle
	{
		friend class Renderer;
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