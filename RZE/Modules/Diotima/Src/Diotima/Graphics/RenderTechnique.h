#pragma once

namespace Diotima
{
	class GFXShaderPipeline;
	class GFXMaterial;

	class GFXRenderTechnique
	{

	private:
		GFXShaderPipeline* mShaderGroup;
		GFXMaterial* mMaterial;
	};
}
