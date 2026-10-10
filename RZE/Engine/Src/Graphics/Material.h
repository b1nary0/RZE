#pragma once

#include <Rendering/BufferHandle.h>

#include <Asset/AssetImport/MaterialAssetImporter.h>

#include <array>

class PixelShader;
class ShaderTechnique;
class Texture2D;

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
	};

public:
	MaterialInstance(const std::string& name);
	~MaterialInstance();
	
public:
	// @TODO hoist MaterialData out and use it in MaterialAssetImporter and MaterialInstance (no namespace resolution)
	static std::shared_ptr<MaterialInstance> Create(const MaterialAssetImporter::MaterialData& materialData);

	void SetTexture(U8 textureSlot, const ResourceHandle& textureResource);

	const ResourceHandle& GetTexture(U8 textureSlot) const;
	// The loaded texture in a slot, or nullptr if the slot is empty. Use this when drawing: it's resolved
	// when the slot is set, so it doesn't look the resource up by path.
	const Texture2D* GetTextureResource(U8 textureSlot) const;

	const std::string& GetName() const { return m_name; }

	// @TODO SetShaderTechnique is actually being set as PixelShader directly until actual techniques are implemented
	void SetShaderTechnique(const ResourceHandle& shaderTechnique);
	const ResourceHandle& GetShaderResource() const { return m_shaderTechnique; }
	// The loaded pixel shader, resolved when the technique is set. Use this when drawing.
	const PixelShader* GetPixelShader() const { return m_pixelShader; }

	MaterialParams& GetProperties() { return m_properties; }
	const MaterialParams& GetProperties() const { return m_properties; }

	// @TODO Is this API good?
	// Commit property changes to GPU buffer
	void CommitPropertyChanges();

	const Rendering::ConstantBufferHandle& GetParamBuffer() const;
		
private:
	std::string m_name;

	MaterialParams m_properties;

	ResourceHandle m_shaderTechnique;
	std::vector<ResourceHandle> m_textureSlots;

	// Resolved from the handles above when they're set. The handles keep the resources loaded, so these stay valid.
	const PixelShader* m_pixelShader = nullptr;
	std::array<const Texture2D*, TEXTURE_SLOT_COUNT> m_textureResources {};

	Rendering::ConstantBufferHandle m_paramBuffer;
};
