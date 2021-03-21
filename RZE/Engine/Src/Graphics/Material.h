#pragma once

class Texture2D;

// [newrenderer]
// This class (well, material/shader system in general...) needs a lot of thought.
// How to approach this class? Should we have:
// 1) MaterialComponent which will fill out internal data based on a .material proprietary format?
// 2) Just add a shader to this? Where do shaders live? How are they represented engine-side vs renderer side?
class Material
{
	friend class MaterialDatabase;

public:
	~Material();

private:
	Material();

public:
	void SetDiffuse(Texture2D* texture);
	void SetSpecular(Texture2D* texture);
	void SetNormal(Texture2D* texture);

public:
	const Texture2D& GetDiffuse() const;
	const Texture2D& GetSpecular() const;
	const Texture2D& GetNormal() const;

	bool HasDiffuse() const;
	bool HasSpecular() const;
	bool HasNormal() const;

public:
	bool IsTextured() const;

public:
	// #TODO(Josh::Turn into material properties or something; material system?)
	float Shininess { 1.0f };
	float Opacity { 1.0f };

private:
	Texture2D* mDiffuseMap;
	Texture2D* mSpecularMap;
	Texture2D* mNormalMap;
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
	Material* GetOrCreateMaterial(const std::string& name)
	{
		auto iter = mDatabase.find(name);

		if (iter == mDatabase.end())
		{
			Material* material = new Material();
			mDatabase[name] = material;
			return material;
		}

		return iter->second;
	}

	Material* FindMaterial(const std::string& name)
	{
		auto iter = mDatabase.find(name);
		if (iter != mDatabase.end())
		{
			return iter->second;
		}
		return nullptr;
	}

private:
	std::unordered_map<std::string, Material*> mDatabase;
};
