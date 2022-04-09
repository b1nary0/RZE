#include <StdAfx.h>
#include <Graphics/Material.h>

#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>

#include <Rendering/Renderer.h>

#include <Utils/Memory/MemoryUtils.h>

MaterialInstance::MaterialInstance(const std::string& name)
	: m_name(name)
{
	m_textureSlots.resize(TEXTURE_SLOT_COUNT);
	
	m_paramBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, MemoryUtils::AlignSize(sizeof(MaterialParams), 16), 1);
}

MaterialInstance::~MaterialInstance()
{
}

std::shared_ptr<MaterialInstance> MaterialInstance::Create(const MaterialAssetImporter::MaterialData& materialData)
{
	std::shared_ptr<MaterialInstance> materialInstance = std::make_shared<MaterialInstance>(materialData.MaterialName);

	U8 textureSlot = 0; // #TODO This is a really hacky way to do this. Not safe at all in any other circumstance
	for (const std::string& texturePath : materialData.TexturePaths)
	{
		if (!texturePath.empty())
		{
			materialInstance->SetTexture(textureSlot++, RZE::GetResourceHandler().LoadResource<Texture2D>(FilePath(texturePath)));
		}
		else
		{
			textureSlot++;
		}
	}

	// @TODO This needs to be reworked. Should have the shader linked with the material asset?
	if ((materialData.TextureFlags & MaterialAssetImporter::MaterialData::TEXTUREFLAG_ALL) == MaterialAssetImporter::MaterialData::TEXTUREFLAG_ALL)
	{
		FilePath fullShaderPath("Assets/Shaders/Pixel_NewRenderer.hlsl");
		materialInstance->SetShaderTechnique(RZE::GetResourceHandler().LoadResource<PixelShader>(fullShaderPath, "Pixel_NewRenderer"));
	}
	else if (materialData.TextureFlags == MaterialAssetImporter::MaterialData::TEXTUREFLAG_NONE)
	{
		FilePath noTextureShaderPath("Assets/Shaders/Pixel_Default_NewRenderer.hlsl");
		materialInstance->SetShaderTechnique(RZE::GetResourceHandler().LoadResource<PixelShader>(noTextureShaderPath, "Pixel_Default_NewRenderer"));
	}
	else
	{
		FilePath diffuseOnlyPath("Assets/Shaders/Pixel_NewRenderer_DiffuseOnly.hlsl");
		materialInstance->SetShaderTechnique(RZE::GetResourceHandler().LoadResource<PixelShader>(diffuseOnlyPath, "Pixel_NewRenderer_DiffuseOnly"));
	}

	materialInstance->GetProperties().Shininess = materialData.Properties.Shininess;
	materialInstance->GetProperties().Opacity = materialData.Properties.Opacity;
	materialInstance->CommitPropertyChanges();

	return materialInstance;
}

void MaterialInstance::SetTexture(U8 textureSlot, const ResourceHandle& textureResource)
{
	AssertExpr(textureSlot < TEXTURE_SLOT_COUNT);
	m_textureSlots[textureSlot] = textureResource;
}

const ResourceHandle& MaterialInstance::GetTexture(U8 textureSlot) const
{
	AssertExpr(textureSlot < TEXTURE_SLOT_COUNT);
	return m_textureSlots[textureSlot];
}

void MaterialInstance::SetShaderTechnique(const ResourceHandle& shaderTechnique)
{
	m_shaderTechnique = shaderTechnique;
}

void MaterialInstance::CommitPropertyChanges()
{
	Rendering::Renderer::UploadDataToBuffer(m_paramBuffer, &m_properties);
}

Rendering::ConstantBufferHandle MaterialInstance::GetParamBuffer() const
{
	return m_paramBuffer;
}
