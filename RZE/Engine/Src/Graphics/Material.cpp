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
	m_paramBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(GraphicsLayouts::MaterialBufferLayout), 16, 1);
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
	TypedResourceHandle<PixelShader> LoadShader(MaterialInstance::EShader shader)
	{
		// @TODO This needs to be reworked. Should have the shader linked with the material asset?
		switch (shader)
		{
		case MaterialInstance::EShader::NormalMapped:
			return LoadPixelShader("Pixel_NewRenderer");
		case MaterialInstance::EShader::Untextured:
			// @TODO Understand why (at least) Material_47 on sponza object has NO textures - how is this even worth it? If we don't have texures to display what
			// purpose do we have?
			return LoadPixelShader("Pixel_Default_NewRenderer");
		default:
			return LoadPixelShader("Pixel_NewRenderer_DiffuseOnly");
		}
	}
}

std::shared_ptr<MaterialInstance> MaterialInstance::Create(const MaterialAssetImporter::MaterialData& materialData)
{
	std::shared_ptr<MaterialInstance> materialInstance = std::make_shared<MaterialInstance>(materialData.MaterialName);
	materialInstance->SetValues(MaterialValues::FromMaterialData(materialData));
	return materialInstance;
}

GraphicsLayouts::MaterialBufferLayout MaterialInstance::BuildBufferData(const MaterialValues& values)
{
	const bool isBlended = ::IsBlended(values.BlendMode);

	GraphicsLayouts::MaterialBufferLayout params;
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
	params.HasSpecularMap = values.Textures[TEXTURE_SLOT_SPECULAR].IsValid() ? 1.0f : 0.0f;
	return params;
}

MaterialInstance::EShader MaterialInstance::ChooseShader(const MaterialValues& values)
{
	// Only that shader applies normal maps; it reads the diffuse and specular maps only if they're there
	if (values.Textures[TEXTURE_SLOT_NORMAL].IsValid())
	{
		return EShader::NormalMapped;
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

	m_pixelShader = LoadShader(ChooseShader(m_values));

	m_bufferData = BuildBufferData(m_values);
	CommitPropertyChanges();
}

void MaterialInstance::SetTexture(U8 textureSlot, const ResourceHandle& textureResource)
{
	AssertExpr(textureSlot < TEXTURE_SLOT_COUNT);
	m_textures[textureSlot] = TypedResourceHandle<Texture2D>(textureResource);
}

const ResourceHandle& MaterialInstance::GetTexture(U8 textureSlot) const
{
	AssertExpr(textureSlot < TEXTURE_SLOT_COUNT);
	return m_textures[textureSlot].GetHandle();
}

const Texture2D* MaterialInstance::GetTextureResource(U8 textureSlot) const
{
	AssertExpr(textureSlot < TEXTURE_SLOT_COUNT);
	return m_textures[textureSlot].Get();
}

void MaterialInstance::SetShaderTechnique(const ResourceHandle& shaderTechnique)
{
	m_pixelShader = TypedResourceHandle<PixelShader>(shaderTechnique);
}

void MaterialInstance::CommitPropertyChanges()
{
	Rendering::Renderer::UploadDataToBuffer<GraphicsLayouts::MaterialBufferLayout>(m_paramBuffer, &m_bufferData);
}

const Rendering::ConstantBufferHandle& MaterialInstance::GetParamBuffer() const
{
	return m_paramBuffer;
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
			return std::string(GetBlendModeName(material.GetBlendMode()));
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
					textures.push_back(MakeObjectRef(texture, GetTextureSlotName(static_cast<ETextureSlot>(textureSlot))));
				}
			}
			return textures;
		});
}
