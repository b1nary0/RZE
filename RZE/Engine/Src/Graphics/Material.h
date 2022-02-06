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
class Material
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
	struct MaterialProperties
	{
		float Shininess{ 1.0f };
		float Opacity{ 1.0f };
	};

public:
	~Material();

private:
	Material();

public:
	void SetTexture(U8 textureSlot, const ResourceHandle& textureResource);

	ResourceHandle GetTexture(U8 textureSlot);
	const ResourceHandle& GetTexture(U8 textureSlot) const;

	// @TODO SetShaderTechnique is actually being set as PixelShader directly until actual techniques are implemented
	void SetShaderTechnique(const ResourceHandle& shaderTechnique);
	ResourceHandle GetShaderResource() const { return mShaderTechnique; }

	MaterialProperties& GetProperties() { return m_properties; }
	const MaterialProperties& GetProperties() const { return m_properties; }
		
private:
	MaterialProperties m_properties;

	ResourceHandle mShaderTechnique;
	std::vector<ResourceHandle> mTextureSlots;
};

// [newrenderer]
// Manages material resources so we only ever load what we need to texture-wise
// Just an idea, will need mega fleshing out
class MaterialDatabase
{
public:
	MaterialDatabase() = default;
	~MaterialDatabase() = default;

public:
	static MaterialDatabase& Get()
	{
		static MaterialDatabase staticRef;
		return staticRef;
	}

public:
	// Slow and gross but just stubbing code ideas for the moment
	std::shared_ptr<Material> GetOrCreateMaterial(const std::string& name)
	{
		auto iter = mDatabase.find(name);

		if (iter == mDatabase.end())
		{
			std::shared_ptr<Material> material = std::shared_ptr<Material>(new Material());
			mDatabase[name] = material;
			return std::move(material);
		}

		return iter->second;
	}

	std::shared_ptr<Material> FindMaterial(const std::string& name)
	{
		auto iter = mDatabase.find(name);
		if (iter != mDatabase.end())
		{
			return iter->second;
		}
		return nullptr;
	}

private:
	std::unordered_map<std::string, std::shared_ptr<Material>> mDatabase;
};
