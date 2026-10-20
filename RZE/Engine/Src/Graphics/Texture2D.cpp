#include <StdAfx.h>
#include <Graphics/Texture2D.h>

#include <Game/World/GameObject/Properties/PropertyBuilder.h>

#include <Rendering/Renderer.h>
#include <Rendering/Driver/GFXBuffer.h>

#include <STB/stb_image.h>

Texture2D::Texture2D()
	: IResource()
	, m_width(0)
	, m_height(0)
	, m_channels(0)
{
}

Texture2D::~Texture2D()
{
}

bool Texture2D::Load(const Filepath& filePath)
{
	return LoadCPU(filePath) && FinalizeStep();
}

bool Texture2D::Load(Rendering::TexelBuffer&& texels, int width, int height)
{
	m_texels = std::move(texels);
	m_width = width;
	m_height = height;

	const bool hasTexels = !m_texels.IsEmpty();
	CreateGPUResource();

	return hasTexels;
}

bool Texture2D::LoadCPU(const Filepath& filePath)
{
	m_filepath = filePath;

	// #TODO(Josh) This will need to be customized for different bit sizes... 24, 32 etc?
	m_texels = Rendering::TexelBuffer(stbi_load(filePath.GetAbsolutePath().c_str(), &m_width, &m_height, &m_channels, STBI_rgb_alpha), &stbi_image_free);
	if (m_texels.IsEmpty())
	{
		RZE_LOG_ARGS("Loading texture from [%s] failed.", filePath.GetRelativePath().c_str());
		return false;
	}

	return true;
}

ResourceFinalizeCost Texture2D::GetNextFinalizeStepCost() const
{
	// Texel data isn't copied into the command arena, only the command itself.
	ResourceFinalizeCost cost;
	cost.ArenaBytes = 256;
	cost.UploadBytes = static_cast<size_t>(m_width) * static_cast<size_t>(m_height) * 4;
	return cost;
}

bool Texture2D::FinalizeStep()
{
	AssertExpr(!m_texels.IsEmpty());
	CreateGPUResource();
	return true;
}

void Texture2D::CreateGPUResource()
{
	Rendering::GFXTextureBufferParams params = { 0 };
	params.bIsRenderTarget = true;
	params.bIsShaderResource = true;
	params.Height = m_height;
	params.Width = m_width;
	params.MipLevels = 0;
	params.MostDetailedMip = 0;
	params.SampleCount = 1;
	params.SampleQuality = 0;
	// The upload takes the texels and frees them once the render thread has made the texture
	m_GPUResource = Rendering::Renderer::CreateTextureBuffer2D(std::move(m_texels), params);
}

void Texture2D::Release()
{
	// Only still here if it was never uploaded, e.g. a load that was given up after LoadCPU
	m_texels.Reset();
}

bool Texture2D::IsOnGPU() const
{
	return m_GPUResource.IsValid();
}

Vector2D Texture2D::GetDimensions() const
{
	return Vector2D(m_width, m_height);
}

void Texture2D::DescribeProperties(PropertyBuilder<Texture2D>& properties)
{
	properties.AssetPath("File", &Texture2D::GetFilepath);
	properties.Value<PropertyTexture>("Preview", [](const Texture2D& texture)
		{
			PropertyTexture preview;
			preview.Handle = texture.GetPlatformObject();
			preview.Size = texture.GetDimensions();
			return preview;
		});
}
