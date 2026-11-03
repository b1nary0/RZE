#pragma once

#include <Rendering/Driver/ShaderTypes.h>

namespace Rendering
{
	class GLDevice;

	class GLVertexShader : public IVertexShader
	{
	public:
		explicit GLVertexShader(GLDevice& device);

	// IVertexShader interface
	public:
		void Create(const Filepath& filePath, const ShaderInputLayout& inputLayout) override;
		void Release() override;
		void SetActive() override;
		void SetInputLayout() override;

	private:
		[[maybe_unused]] GLDevice& m_device;
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

	private:
		[[maybe_unused]] GLDevice& m_device;
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

	private:
		[[maybe_unused]] GLDevice& m_device;
	};
}
