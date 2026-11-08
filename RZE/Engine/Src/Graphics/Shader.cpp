#include <Graphics/Shader.h>

#include <Asset/AssetPaths.h>

#include <Graphics/RenderEngine.h>

#include <Graphics/Material.h>

#include <Rendering/Renderer.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Memory/MemoryUtils.h>

bool VertexShader::Load(const Filepath& filePath)
{
	m_shader = Rendering::Renderer::CreateVertexShader(filePath, m_shaderInputLayout);
	m_cameraDataBuf = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(RenderCamera), 128, 1);
	m_worldMatrixBuf = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(Matrix4x4), 128, 1);

	return m_shader.IsValid();
}

void VertexShader::Release()
{
	Rendering::Renderer::ReleaseVertexShader(m_shader);
}

bool PixelShader::Load(const Filepath& filePath)
{
	m_shader = Rendering::Renderer::CreatePixelShader(filePath);

	return m_shader.IsValid();
}

void PixelShader::Release()
{
	Rendering::Renderer::ReleasePixelShader(m_shader);
}

const Rendering::PixelShaderHandle& PixelShader::GetPlatformObject() const
{
	return m_shader;
}

bool ComputeShader::Load(const Filepath& filePath)
{
	m_shader = Rendering::Renderer::CreateComputeShader(filePath);

	return m_shader.IsValid();
}

void ComputeShader::Release()
{
	Rendering::Renderer::ReleaseComputeShader(m_shader);
}

TypedResourceHandle<VertexShader> LoadVertexShader(const std::string& name, const Rendering::ShaderInputLayout& inputLayout)
{
	ResourceHandle handle = RZE().GetResourceHandler().LoadResource<VertexShader>(AssetPaths::GetShader(name), name, inputLayout);
	AssertExpr(handle.IsValid());
	return TypedResourceHandle<VertexShader>(std::move(handle));
}

TypedResourceHandle<PixelShader> LoadPixelShader(const std::string& name)
{
	ResourceHandle handle = RZE().GetResourceHandler().LoadResource<PixelShader>(AssetPaths::GetShader(name), name);
	AssertExpr(handle.IsValid());
	return TypedResourceHandle<PixelShader>(std::move(handle));
}

TypedResourceHandle<ComputeShader> LoadComputeShader(const std::string& name)
{
	ResourceHandle handle = RZE().GetResourceHandler().LoadResource<ComputeShader>(AssetPaths::GetShader(name), name);
	AssertExpr(handle.IsValid());
	return TypedResourceHandle<ComputeShader>(std::move(handle));
}
