#pragma once

#include <Rendering/BufferHandle.h>

#include <EngineCore/Resources/ResourceHandler.h>

#include <Asset/AssetImport/MaterialAssetImporter.h>

#include <Graphics/MaterialValues.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

class PixelShader;
class ShaderTechnique;
class Texture2D;
template <typename TObject>
class PropertyBuilder;

// [newrenderer]
// This class (well, material/shader system in general...) needs a lot of thought.
// How to approach this class? Should we have:
// 1) MaterialComponent which will fill out internal data based on a .material proprietary format?
// 2) Just add a shader to this? Where do shaders live? How are they represented engine-side vs renderer side?

// This class needs a UID that we can use to sort in renderer.
class MaterialInstance
{
	friend class MaterialDatabase;
public:
	enum TextureSlot : U8
	{
		TEXTURE_SLOT_DIFFUSE,
		TEXTURE_SLOT_SPECULAR,
		TEXTURE_SLOT_NORMAL,
		TEXTURE_SLOT_OPACITY,
		TEXTURE_SLOT_HEIGHT,
		TEXTURE_SLOT_COUNT
	};
	static_assert(TEXTURE_SLOT_COUNT == MaterialValues::k_textureSlotCount, "MaterialValues has a texture per slot");

	// The pixel shader a material draws with, from which textures it has (ChooseShader)
	enum class EShader
	{
		// No textures
		Untextured,
		// Some textures, but not all of diffuse, specular and normal
		DiffuseOnly,
		// Diffuse, specular and normal
		Full
	};

	// #TODO(Josh::Turn into material properties or something; material system?)
	struct MaterialParams
	{
		float Shininess{ 1.0f };
		float Opacity{ 1.0f };
		// 1.0f when the matching texture slot is set. Unset slots aren't unbound, so the shader needs
		// these to avoid sampling a previous draw's texture.
		float HasOpacityMask{ 0.0f };
		float HasHeightMap{ 0.0f };
		// 1.0f when the opacity mask is in the texture's alpha channel rather than red
		float OpacityMaskInAlpha{ 0.0f };
		// 1.0f when the mask's white means see-through rather than solid
		float InvertOpacityMask{ 0.0f };
		// 1.0f when the mask is how much of the material shows (blended materials) rather than where it's cut away
		float OpacityMaskIsCoverage{ 0.0f };
		// 1.0f when the diffuse slot is set; without it the tint alone is the colour. 1.0f by default, since materials
		// made in code set their textures themselves (SetTexture) and keep the defaults.
		float HasDiffuseMap{ 1.0f };
		// MaterialValues::Tint; HLSL starts a float3 on a new 16-byte register, as here
		float Tint[3]{ 1.0f, 1.0f, 1.0f };
		// Keeps the buffer a whole number of 16-byte registers, which is what's uploaded
		float _pad0{ 0.0f };
	};

public:
	MaterialInstance(const std::string& name);
	~MaterialInstance();
	
public:
	// A material asset's material (MaterialValues::FromMaterialData, then SetValues)
	static std::shared_ptr<MaterialInstance> Create(const MaterialAssetImporter::MaterialData& materialData);

	// The GPU parameters values draw with. Plain values in and out, so it's testable without a GPU.
	static MaterialParams BuildParams(const MaterialValues& values);
	// The shader values draw with. Plain values in and out, so it's testable without a GPU.
	static EShader ChooseShader(const MaterialValues& values);

	// For tools: the material's shader and textures
	static void DescribeProperties(PropertyBuilder<MaterialInstance>& properties);

	static const char* GetTextureSlotName(TextureSlot textureSlot);

	// The one way a material described by values changes: loads its textures and shader, and works out and uploads
	// its GPU parameters, so nothing derived from the values can go stale
	void SetValues(const MaterialValues& values);
	const MaterialValues& GetValues() const { return m_values; }

	// For materials made in code from textures that aren't files (e.g. GifChatComponent's frames). SetValues sets
	// the textures and shader itself.
	void SetTexture(U8 textureSlot, const ResourceHandle& textureResource);

	const ResourceHandle& GetTexture(U8 textureSlot) const;
	// The loaded texture in a slot, or nullptr if the slot is empty. Use this when drawing: it's resolved
	// when the slot is set, so it doesn't look the resource up by path.
	const Texture2D* GetTextureResource(U8 textureSlot) const;

	const std::string& GetName() const { return m_name; }

	// @TODO SetShaderTechnique is actually being set as PixelShader directly until actual techniques are implemented
	// For materials made in code, like SetTexture
	void SetShaderTechnique(const ResourceHandle& shaderTechnique);
	const ResourceHandle& GetShaderResource() const { return m_shaderTechnique; }
	// The loaded pixel shader, resolved when the technique is set. Use this when drawing.
	const PixelShader* GetPixelShader() const { return m_pixelShader; }

	using EBlendMode = MaterialAssetImporter::MaterialData::EBlendMode;
	EBlendMode GetBlendMode() const { return m_values.BlendMode; }
	// Drawn in the forward pass's sorted, blended pass rather than with the opaque draws
	bool IsBlended() const { return MaterialAssetImporter::IsBlended(m_values.BlendMode); }

	// @TODO Is this API good?
	// Commit property changes to GPU buffer
	void CommitPropertyChanges();

	const Rendering::ConstantBufferHandle& GetParamBuffer() const;
		
private:
	std::string m_name;

	// What SetValues was last given; the defaults for a material made in code
	MaterialValues m_values;
	// Worked out from m_values (BuildParams), except for materials made in code, which keep the defaults
	MaterialParams m_properties;

	ResourceHandle m_shaderTechnique;
	std::vector<ResourceHandle> m_textureSlots;

	// Resolved from the handles above when they're set. The handles keep the resources loaded, so these stay valid.
	const PixelShader* m_pixelShader = nullptr;
	std::array<const Texture2D*, TEXTURE_SLOT_COUNT> m_textureResources {};

	Rendering::ConstantBufferHandle m_paramBuffer;
};
