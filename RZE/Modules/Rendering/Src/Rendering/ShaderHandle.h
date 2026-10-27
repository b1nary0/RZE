#pragma once

#include <Rendering/Driver/ShaderTypes.h>

#include <memory>

namespace Rendering
{
	class VertexShaderHandle
	{
		friend class Renderer;
		friend class RenderThread;

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
		friend class RenderThread;

	public:
		PixelShaderHandle() = default;
		virtual ~PixelShaderHandle() = default;

		bool IsValid() { return m_shader != nullptr; }

	private:
		PixelShaderHandle(const std::shared_ptr<IPixelShader>& shader)
			: m_shader(shader) {}

		std::shared_ptr<IPixelShader> m_shader;
	};

	class ComputeShaderHandle
	{
		friend class Renderer;
		friend class RenderThread;

	public:
		ComputeShaderHandle() = default;
		virtual ~ComputeShaderHandle() = default;

		bool IsValid() { return m_shader != nullptr; }

	private:
		ComputeShaderHandle(const std::shared_ptr<IComputeShader>& shader)
			: m_shader(shader) {}

		std::shared_ptr<IComputeShader> m_shader;
	};
}