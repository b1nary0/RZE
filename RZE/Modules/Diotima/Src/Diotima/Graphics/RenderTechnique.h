#pragma once

namespace Diotima
{
	class GFXShaderPipeline;
	class GFXMaterial;

	class GFXRenderTechnique
	{
	public:
		GFXRenderTechnique();
		GFXRenderTechnique(GFXShaderPipeline* const shaderPipeline, GFXMaterial* const material);

	private:
		GFXShaderPipeline* mShaderPipeline;
		GFXMaterial* mMaterial;
	};
}
