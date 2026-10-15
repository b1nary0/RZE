#include <StdAfx.h>
#include <Graphics/Material.h>

#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>

#include <Game/World/GameObject/Properties/PropertyBuilder.h>

#include <Rendering/Renderer.h>

#include <Utils/Memory/MemoryUtils.h>

MaterialInstance::MaterialInstance(const std::string& name)
	: m_name(name)
{
	m_textureSlots.resize(TEXTURE_SLOT_COUNT);
	
	m_paramBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(MaterialParams), 16, 1);
	// The GPU starts with the defaults too, so a material nobody commits (e.g. one made in code) isn't drawn with an
	// empty buffer: its Opacity would be 0, which the shaders write as alpha, and post-processing then treats it as
	// background
	CommitPropertyChanges();
}

MaterialInstance::~MaterialInstance()
{
}

namespace
{
	ResourceHandle LoadShader(MaterialInstance::EShader shader)
	{
		// @TODO This needs to be reworked. Should have the shader linked with the material asset?
		switch (shader)
		{
		case MaterialInstance::EShader::Full:
			return RZE().GetResourceHandler().LoadResource<PixelShader>(Filepath("Assets/Shaders/Pixel_NewRenderer.hlsl"), "Pixel_NewRenderer");
		case MaterialInstance::EShader::Untextured:
			// @TODO Understand why (at least) Material_47 on sponza object has NO textures - how is this even worth it? If we don't have texures to display what
			// purpose do we have?
			return RZE().GetResourceHandler().LoadResource<PixelShader>(Filepath("Assets/Shaders/Pixel_Default_NewRenderer.hlsl"), "Pixel_Default_NewRenderer");
		default:
			return RZE().GetResourceHandler().LoadResource<PixelShader>(Filepath("Assets/Shaders/Pixel_NewRenderer_DiffuseOnly.hlsl"), "Pixel_NewRenderer_DiffuseOnly");
		}
	}
}

std::shared_ptr<MaterialInstance> MaterialInstance::Create(const MaterialAssetImporter::MaterialData& materialData)
{
	std::shared_ptr<MaterialInstance> materialInstance = std::make_shared<MaterialInstance>(materialData.MaterialName);
	materialInstance->SetValues(MaterialValues::FromMaterialData(materialData));
	return materialInstance;
}

MaterialInstance::MaterialParams MaterialInstance::BuildParams(const MaterialValues& values)
{
	const bool isBlended = MaterialAssetImporter::IsBlended(values.BlendMode);

	MaterialParams params;
	params.Shininess = values.Shininess;
	// The shaders write opacity as the scene's alpha, which PostProcess also reads (background, exposure weight), so
	// only a material drawn blended gets anything but solid
	params.Opacity = isBlended ? values.Opacity : 1.0f;
	params.HasOpacityMask = values.Textures[TEXTURE_SLOT_OPACITY].IsValid() ? 1.0f : 0.0f;
	params.HasHeightMap = values.Textures[TEXTURE_SLOT_HEIGHT].IsValid() ? 1.0f : 0.0f;
	params.OpacityMaskInAlpha = values.OpacityMaskInAlpha ? 1.0f : 0.0f;
	params.InvertOpacityMask = values.InvertOpacityMask ? 1.0f : 0.0f;
	// A cutout clips at the mask; a blended material fades by it
	params.OpacityMaskIsCoverage = isBlended ? 1.0f : 0.0f;
	params.HasDiffuseMap = values.Textures[TEXTURE_SLOT_DIFFUSE].IsValid() ? 1.0f : 0.0f;
	params.Tint[0] = values.Tint.X();
	params.Tint[1] = values.Tint.Y();
	params.Tint[2] = values.Tint.Z();
	return params;
}

MaterialInstance::EShader MaterialInstance::ChooseShader(const MaterialValues& values)
{
	const bool hasDiffuse = values.Textures[TEXTURE_SLOT_DIFFUSE].IsValid();
	const bool hasSpecular = values.Textures[TEXTURE_SLOT_SPECULAR].IsValid();
	const bool hasNormal = values.Textures[TEXTURE_SLOT_NORMAL].IsValid();
	if (hasDiffuse && hasSpecular && hasNormal)
	{
		return EShader::Full;
	}

	for (const Filepath& texture : values.Textures)
	{
		if (texture.IsValid())
		{
			return EShader::DiffuseOnly;
		}
	}
	return EShader::Untextured;
}

void MaterialInstance::SetValues(const MaterialValues& values)
{
	m_values = values;

	ResourceHandler& resourceHandler = RZE().GetResourceHandler();
	for (U8 textureSlot = 0; textureSlot < TEXTURE_SLOT_COUNT; ++textureSlot)
	{
		const Filepath& texturePath = m_values.Textures[textureSlot];
		SetTexture(textureSlot, texturePath.IsValid() ? resourceHandler.LoadResource<Texture2D>(texturePath) : ResourceHandle::EmptyHandle());
	}

	SetShaderTechnique(LoadShader(ChooseShader(m_values)));

	m_properties = BuildParams(m_values);
	CommitPropertyChanges();
}

void MaterialInstance::SetTexture(U8 textureSlot, const ResourceHandle& textureResource)
{
	AssertExpr(textureSlot < TEXTURE_SLOT_COUNT);
	m_textureSlots[textureSlot] = textureResource;
	m_textureResources[textureSlot] = textureResource.IsValid()
		? RZE().GetResourceHandler().GetResource<Texture2D>(textureResource)
		: nullptr;
}

const ResourceHandle& MaterialInstance::GetTexture(U8 textureSlot) const
{
	AssertExpr(textureSlot < TEXTURE_SLOT_COUNT);
	return m_textureSlots[textureSlot];
}

const Texture2D* MaterialInstance::GetTextureResource(U8 textureSlot) const
{
	AssertExpr(textureSlot < TEXTURE_SLOT_COUNT);
	return m_textureResources[textureSlot];
}

void MaterialInstance::SetShaderTechnique(const ResourceHandle& shaderTechnique)
{
	m_shaderTechnique = shaderTechnique;
	m_pixelShader = shaderTechnique.IsValid()
		? RZE().GetResourceHandler().GetResource<PixelShader>(shaderTechnique)
		: nullptr;
}

void MaterialInstance::CommitPropertyChanges()
{
	Rendering::Renderer::UploadDataToBuffer<MaterialParams>(m_paramBuffer, &m_properties);
}

const Rendering::ConstantBufferHandle& MaterialInstance::GetParamBuffer() const
{
	return m_paramBuffer;
}

const char* MaterialInstance::GetTextureSlotName(TextureSlot textureSlot)
{
	switch (textureSlot)
	{
	case TEXTURE_SLOT_DIFFUSE:
		return "Diffuse";
	case TEXTURE_SLOT_SPECULAR:
		return "Specular";
	case TEXTURE_SLOT_NORMAL:
		return "Normal";
	case TEXTURE_SLOT_OPACITY:
		return "Opacity";
	case TEXTURE_SLOT_HEIGHT:
		return "Height";
	default:
		return "None";
	}
}

void MaterialInstance::DescribeProperties(PropertyBuilder<MaterialInstance>& properties)
{
	properties.DisplayName([](const MaterialInstance& material) { return material.GetName(); });

	properties.Value<std::string>("Shader", [](const MaterialInstance& material)
		{
			const PixelShader* const shader = material.GetPixelShader();
			return shader != nullptr ? shader->GetName() : std::string("(none)");
		});

	// #TODO(Transparency) Read-only for now: material edits have nowhere to be saved, and the plan's editable blend
	// mode needs an enum property type (PropertyBuilder has none)
	properties.Value<std::string>("Blend Mode", [](const MaterialInstance& material)
		{
			return std::string(MaterialAssetImporter::GetBlendModeName(material.GetBlendMode()));
		});
	properties.Value<float>("Opacity", [](const MaterialInstance& material) { return material.GetValues().Opacity; });

	// Only the slots that have a texture, each labelled with its slot
	properties.Array("Textures", [](const MaterialInstance& material)
		{
			std::vector<ObjectRef> textures;
			for (U8 textureSlot = 0; textureSlot < TEXTURE_SLOT_COUNT; ++textureSlot)
			{
				if (const Texture2D* const texture = material.GetTextureResource(textureSlot))
				{
					textures.push_back(MakeObjectRef(texture, GetTextureSlotName(static_cast<TextureSlot>(textureSlot))));
				}
			}
			return textures;
		});
}
