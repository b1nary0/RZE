#pragma once

class Texture2D;

class Material
{
public:
	Material();
	~Material();

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