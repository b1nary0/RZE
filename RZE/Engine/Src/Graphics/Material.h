#pragma once

#include <Rendering/BufferHandle.h>

#include <EngineCore/Resources/ResourceHandler.h>
#include <EngineCore/Resources/TypedResourceHandle.h>

#include <Asset/AssetImport/MaterialAssetImporter.h>

#include <Graphics/GraphicsLayouts.h>
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
	// The pixel shader a material draws with, from which textures it has (ChooseShader)
	enum class EShader
	{
		// No textures
		Untextured,
		// Some textures, but no normal map
		DiffuseOnly,
		// A normal map, with or without diffuse and specular maps (HasDiffuseMap, HasSpecularMap)
		NormalMapped
	};

public:
	MaterialInstance(const std::string& name);
	~MaterialInstance();
	
public:
	// A material asset's material (MaterialValues::FromMaterialData, then SetValues)
	static std::shared_ptr<MaterialInstance> Create(const MaterialAssetImporter::MaterialData& materialData);

	// The GPU parameters values draw with. Plain values in and out, so it's testable without a GPU.
	static GraphicsLayouts::MaterialBufferLayout BuildBufferData(const MaterialValues& values);
	// The shader values draw with. Plain values in and out, so it's testable without a GPU.
	static EShader ChooseShader(const MaterialValues& values);

	// For tools: the material's shader and textures
	static void DescribeProperties(PropertyBuilder<MaterialInstance>& properties);

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
	const ResourceHandle& GetShaderResource() const { return m_pixelShader.GetHandle(); }
	// The loaded pixel shader, resolved when the technique is set. Use this when drawing.
	const PixelShader* GetPixelShader() const { return m_pixelShader.Get(); }

	EBlendMode GetBlendMode() const { return m_values.BlendMode; }
	// Drawn in the forward pass's sorted, blended pass rather than with the opaque draws
	bool IsBlended() const { return ::IsBlended(m_values.BlendMode); }

	// @TODO Is this API good?
	// Commit property changes to GPU buffer
	void CommitPropertyChanges();

	const Rendering::ConstantBufferHandle& GetParamBuffer() const;
		
private:
	std::string m_name;

	// What SetValues was last given; the defaults for a material made in code
	MaterialValues m_values;
	// Worked out from m_values (BuildBufferData), except for materials made in code, which keep the defaults
	GraphicsLayouts::MaterialBufferLayout m_bufferData;

	// Each keeps its resource loaded and resolved, so drawing doesn't look anything up by path
	TypedResourceHandle<PixelShader> m_pixelShader;
	std::array<TypedResourceHandle<Texture2D>, TEXTURE_SLOT_COUNT> m_textures;

	Rendering::ConstantBufferHandle m_paramBuffer;
};
