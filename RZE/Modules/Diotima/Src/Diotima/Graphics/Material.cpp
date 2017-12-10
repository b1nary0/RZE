#include <Diotima/Graphics/Material.h>

#include <Diotima/Shaders/ShaderGroup.h>

#include <Utils/DebugUtils/Debug.h>

Diotima::GFXMaterial::GFXMaterial()
{
}

Diotima::GFXMaterial::~GFXMaterial()
{
}

void Diotima::GFXMaterial::Use()
{
	AssertNotNull(mShaderGroup);
	mShaderGroup->Use();
}
