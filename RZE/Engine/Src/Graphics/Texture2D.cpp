#include <StdAfx.h>
#include <Graphics/Texture2D.h>

#include <Rendering/Renderer.h>
#include <Rendering/Driver/GFXBuffer.h>

#include <STB/stb_image.h>

Texture2D::Texture2D()
	: IResource()
	, m_data(nullptr)
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

bool Texture2D::Load(const U8* buffer, int width, int height)
{
	m_data = buffer;
	m_width = width;
	m_height = height;

	CreateGPUResource();

	return m_data != nullptr;
}

bool Texture2D::LoadCPU(const Filepath& filePath)
{
	m_filepath = filePath;

	// #TODO(Josh) This will need to be customized for different bit sizes... 24, 32 etc?
	m_data = stbi_load(filePath.GetAbsolutePath().c_str(), &m_width, &m_height, &m_channels, STBI_rgb_alpha);
	if (m_data == nullptr)
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
	AssertNotNull(m_data);
	CreateGPUResource();
	return true;
}

void Texture2D::CreateGPUResource()
{
	// NOTE: The render thread reads m_data when it processes this command (a frame or so later),
	// so the data must stay alive until then. It is only freed in Release().
	Rendering::GFXTextureBufferParams params = { 0 };
	params.bIsRenderTarget = true;
	params.bIsShaderResource = true;
	params.Height = m_height;
	params.Width = m_width;
	params.MipLevels = 0;
	params.MostDetailedMip = 0;
	params.SampleCount = 1;
	params.SampleQuality = 0;
	m_GPUResource = Rendering::Renderer::CreateTextureBuffer2D(m_data, params);
}

void Texture2D::Release()
{
	if (m_data != nullptr)
	{
		stbi_image_free(const_cast<U8*>(m_data));
		m_data = nullptr;
	}
}

Vector2D Texture2D::GetDimensions() const
{
	return Vector2D(m_width, m_height);
}
