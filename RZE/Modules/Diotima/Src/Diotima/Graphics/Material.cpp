#include <Diotima/Graphics/Material.h>

#include <Diotima/Driver/OpenGL.h>
#include <Diotima/Graphics/Texture2D.h>
#include <Diotima/Shaders/ShaderGroup.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Platform/File.h>

//#include <RapidJSON/document.h>

Diotima::GFXMaterial::GFXMaterial()
{
}

Diotima::GFXMaterial::~GFXMaterial()
{
}

Diotima::GFXShaderGroup* const Diotima::GFXMaterial::GetShaderGroup() const
{
	AssertNotNull(mShaderGroup);
	return mShaderGroup;
}

Diotima::GFXTexture2D* const Diotima::GFXMaterial::GetTexture2D() const
{
	AssertNotNull(mTexture2D);
	return mTexture2D;
}

void Diotima::GFXMaterial::SetShaderGroup(GFXShaderGroup* const shaderGroup)
{
	AssertNotNull(shaderGroup);
	mShaderGroup = shaderGroup;
}

void Diotima::GFXMaterial::SetTexture2D(GFXTexture2D* const texture2D)
{
	AssertNotNull(texture2D);
	mTexture2D = texture2D;
}

bool Diotima::GFXMaterial::Load(const std::string& filePath)
{
// 	File materialFile(filePath);
// 
// 	rapidjson::Document json;
// 	json.Parse(materialFile.Content().c_str());

	return true;
}

void Diotima::GFXMaterial::Use() const
{
	if (mTexture2D)
	{
		OpenGLRHI::Get().BindTexture(EGLCapability::Texture2D, mTexture2D->GetTextureID());
	}

	AssertNotNull(mShaderGroup);
	mShaderGroup->Use();
}
