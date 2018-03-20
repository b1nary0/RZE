#include <Diotima/Graphics/RenderTechnique.h>

namespace Diotima
{
	GFXRenderTechnique::GFXRenderTechnique()
		: mShaderPipeline(nullptr)
		, mMaterial(nullptr)
	{
	}

	GFXRenderTechnique::GFXRenderTechnique(GFXShaderPipeline* const shaderPipeline, GFXMaterial* const material)
		: mShaderPipeline(shaderPipeline)
		, mMaterial(material)
	{
	}
}