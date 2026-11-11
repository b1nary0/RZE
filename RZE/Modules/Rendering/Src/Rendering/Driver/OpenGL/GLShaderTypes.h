#pragma once

#include <Rendering/Driver/ShaderTypes.h>
#include <Rendering/Driver/OpenGL/GLShaderTranslator.h>

#include <vector>

namespace Rendering
{
	class GLDevice;

	// One translated HLSL shader as a separable GL program (GLShaderTranslator), which GLDevice binds into its program
	// pipeline as D3D binds a stage
	class GLShaderProgram
	{
	public:
		GLShaderProgram(GLDevice& device, GLShaderTranslator::EStage stage);
		~GLShaderProgram();

		GLShaderProgram(const GLShaderProgram&) = delete;
		GLShaderProgram& operator=(const GLShaderProgram&) = delete;

		// False, having logged why, if it couldn't be made
		bool Create(const Filepath& filePath);
		void Release();

		// 0 if it couldn't be made
		U32 GetProgram() const { return m_program; }
		GLShaderTranslator::EStage GetStage() const { return m_stage; }
		const GLShaderTranslator::Result& GetTranslation() const { return m_translation; }

	private:
		GLDevice& m_device;
		GLShaderTranslator::EStage m_stage;
		U32 m_program = 0;
		GLShaderTranslator::Result m_translation;
	};

	class GLVertexShader : public IVertexShader
	{
	public:
		// A vertex attribute as the input layout gives it, at the location the shader reads it from
		struct Attribute
		{
			U32 Location = 0;
			EDataFormat Format = EDataFormat::R32G32B32_FLOAT;
			U32 Offset = 0;
			bool IsPerInstance = false;
			// The vertex buffer slot (Renderer::SetVertexBuffer) it's read from
			U32 InputSlot = 0;
		};

		explicit GLVertexShader(GLDevice& device);

	// IVertexShader interface
	public:
		void Create(const Filepath& filePath, const ShaderInputLayout& inputLayout) override;
		void Release() override;
		void SetActive() override;
		void SetInputLayout() override;
		bool IsCreated() const override { return m_program.GetProgram() != 0; }

	public:
		const GLShaderProgram& GetProgram() const { return m_program; }
		const std::vector<Attribute>& GetAttributes() const { return m_attributes; }

	private:
		GLDevice& m_device;
		GLShaderProgram m_program;
		std::vector<Attribute> m_attributes;
	};

	class GLPixelShader : public IPixelShader
	{
	public:
		explicit GLPixelShader(GLDevice& device);

	// IPixelShader interface
	public:
		void Create(const Filepath& filePath) override;
		void Release() override;
		void SetActive() override;

	public:
		const GLShaderProgram& GetProgram() const { return m_program; }

	private:
		GLDevice& m_device;
		GLShaderProgram m_program;
	};

	class GLComputeShader : public IComputeShader
	{
	public:
		explicit GLComputeShader(GLDevice& device);

	// IComputeShader interface
	public:
		void Create(const Filepath& filePath) override;
		void Release() override;
		void SetActive() override;
		bool IsCreated() const override { return m_program.GetProgram() != 0; }

	public:
		const GLShaderProgram& GetProgram() const { return m_program; }

	private:
		GLDevice& m_device;
		GLShaderProgram m_program;
	};
}
