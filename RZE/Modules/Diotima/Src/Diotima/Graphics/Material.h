#pragma once

#include <vector>

#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>

namespace Diotima
{
	class GFXTexture2D;

	class GFXMaterial
	{
	public:
		GFXMaterial();
		~GFXMaterial();

		// #TODO(Josh::Work out what should be exposed in this API)
		void AddTexture(GFXTexture2D* texture);
		bool IsTextured() const;

		const std::vector<GFXTexture2D*>& GetDiffuseTextures() const { return mDiffuseTextures; }
		const std::vector<GFXTexture2D*>& GetSpecularTextures() const { return mSpecularTextures; }
		const std::vector<GFXTexture2D*>& GetNormalMaps() const { return mNormalMaps; }

	private:
		std::vector<GFXTexture2D*> mDiffuseTextures;
		std::vector<GFXTexture2D*> mSpecularTextures;
		std::vector<GFXTexture2D*> mNormalMaps;
	};
}