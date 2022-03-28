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

bool Texture2D::Load(const FilePath& filePath)
{
	m_filepath = filePath;

	// #TODO(Josh) This will need to be customized for different bit sizes... 24, 32 etc?
	m_data = stbi_load(filePath.GetAbsolutePath().c_str(), &m_width, &m_height, &m_channels, STBI_rgb_alpha);
	if (m_data == nullptr)
	{
		LOG_CONSOLE_ARGS("Loading texture from [%s] failed.", filePath.GetRelativePath());
		AssertFalse();
	}

	// @TODO Not sure how happy I am with this being here
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

	return m_data != nullptr;
}

bool Texture2D::Load(const U8* buffer, int width, int height)
{
	m_data = buffer;
	m_width = width;
	m_height = height;

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

	return m_data != nullptr;
}

void Texture2D::Release()
{
	if (m_data != nullptr)
	{
		delete m_data;
		m_data = nullptr;
	}
}

Vector2D Texture2D::GetDimensions() const
{
	return Vector2D(m_width, m_height);
}
