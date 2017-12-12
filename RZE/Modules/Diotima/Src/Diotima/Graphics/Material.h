#pragma once

#include <string>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>

namespace Diotima
{
	class GFXShaderGroup;
	class GFXTexture2D;

	class GFXMaterial
	{
	public:
		GFXMaterial();
		~GFXMaterial();

		GFXShaderGroup* const GetShaderGroup() const;
		GFXTexture2D* const GetTexture2D() const;
		void SetShaderGroup(GFXShaderGroup* const shaderGroup);
		void SetTexture2D(GFXTexture2D* const texture2D);

	public:
		void Use() const;

		Matrix4x4 ModelMat;
		Matrix4x4 ViewMat;
		Matrix4x4 ProjectionMat;
		Vector4D Color;

		Vector3D			ViewPos;
		Vector3D            LightColor;
		Vector3D            LightPos;
		float               LightStrength;

		std::string mName;

		bool operator==(const GFXMaterial& rhs) const
		{
			return mName == rhs.mName;
		}
	private:
		GFXShaderGroup* mShaderGroup;
		GFXTexture2D* mTexture2D; // #NOTE(Josh) will probably move this out later? 2D vs 3D textures

// 
// 		"UModelMat", rend
// 			"UProjectionMat",
// 			"UViewMat", rende
// 
// 			"UFragColor", sDe

	};
}