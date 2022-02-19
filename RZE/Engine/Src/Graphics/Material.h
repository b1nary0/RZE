#pragma once

#include <Rendering/BufferHandle.h>

#include <memory>

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
		TEXTURE_SLOT_COUNT
	};

	// #TODO(Josh::Turn into material properties or something; material system?)
	struct MaterialParams
	{
		float Shininess{ 1.0f };
		float Opacity{ 1.0f };
	};

public:
	MaterialInstance();
	~MaterialInstance();
	
public:
	void SetTexture(U8 textureSlot, const ResourceHandle& textureResource);

	ResourceHandle GetTexture(U8 textureSlot);
	const ResourceHandle& GetTexture(U8 textureSlot) const;

	// @TODO SetShaderTechnique is actually being set as PixelShader directly until actual techniques are implemented
	void SetShaderTechnique(const ResourceHandle& shaderTechnique);
	ResourceHandle GetShaderResource() const { return m_shaderTechnique; }

	MaterialParams& GetProperties() { return m_properties; }
	const MaterialParams& GetProperties() const { return m_properties; }

	// @TODO Is this API good?
	// Commit property changes to GPU buffer
	void CommitPropertyChanges();

	Rendering::ConstantBufferHandle GetParamBuffer() const;
		
private:
	MaterialParams m_properties;

	ResourceHandle m_shaderTechnique;
	std::vector<ResourceHandle> m_textureSlots;

	Rendering::ConstantBufferHandle m_paramBuffer;
};
