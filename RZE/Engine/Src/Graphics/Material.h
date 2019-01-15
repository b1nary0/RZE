#pragma once

class Texture2D;

class Material
{
public:
	Material();
	~Material();

private:
	Texture2D* mDiffuseMap;
	Texture2D* mSpecularMap;
	Texture2D* mNormalMap;
};