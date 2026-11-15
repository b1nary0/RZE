#pragma once

#include <Rendering/BufferHandle.h>

#include <EngineCore/Resources/ResourceHandler.h>
#include <EngineCore/Resources/TypedResourceHandle.h>

#include <Asset/AssetImport/MaterialAssetImporter.h>

#include <Graphics/GraphicsLayouts.h>
#include <Graphics/MaterialValues.h>

#include <array>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

class PixelShader;
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

	// Where the pixel shaders read a material's buffer (MaterialBuffer in Common/PixelResources.hlsli)
	static constexpr U32 k_bufferSlot = 1;

public:
	MaterialInstance(const std::string& name);
	~MaterialInstance();
	
public:
	// A material asset's material (MaterialValues::FromMaterialData, then SetValues)
	static std::shared_ptr<MaterialInstance> Create(const MaterialAssetImporter::MaterialData& materialData);

	// The GPU parameters every draw binding a material with these values reads from its buffer. Plain values in and
	// out, so it's testable without a GPU.
	static GraphicsLayouts::MaterialBufferLayout BuildBufferData(const MaterialValues& values);
	// The GPU parameters each draw carries itself: the values an object can override and still draw the same
	// material (MaterialSlot::BuildInstanceData), so they're never read from a material's buffer. Only a blended
	// material draws with its opacity; the rest are solid.
	static GraphicsLayouts::InstanceMaterialLayout BuildInstanceData(const MaterialValues& values);
	// The same from the values alone, for a caller that has them from somewhere else
	static GraphicsLayouts::InstanceMaterialLayout BuildInstanceData(const Vector3D& tint, float shininess, float opacity, EBlendMode blendMode);
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
	// It has an opacity texture, whatever its blend mode: a blended material can have one too, so this isn't
	// EBlendMode::Cutout
	bool HasOpacityMap() const;

	const std::string& GetName() const { return m_name; }

	// For materials made in code, like SetTexture. SetValues sets the shader itself.
	void SetPixelShader(TypedResourceHandle<PixelShader>&& pixelShader);
	// The loaded pixel shader, resolved when the technique is set. Use this when drawing.
	const PixelShader* GetPixelShader() const { return m_pixelShader.Get(); }

	EBlendMode GetBlendMode() const { return m_values.BlendMode; }
	// Drawn in the forward pass's sorted, blended pass rather than with the opaque draws
	bool IsBlended() const { return ::IsBlended(m_values.BlendMode); }

	// @TODO Is this API good?
	// Commit property changes to GPU buffer
	void CommitPropertyChanges();

	// Binds what a pixel shader reads of the material for a draw: its buffer, and each texture it has, in that
	// texture's slot. An empty slot keeps whatever was bound there.
	void Bind() const;
	// As above, with only the given textures, for shaders that read fewer
	void Bind(std::initializer_list<ETextureSlot> textureSlots) const;
		
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

private:
	// Binds the texture in a slot, if it has one
	void BindTexture(ETextureSlot textureSlot) const;
};
