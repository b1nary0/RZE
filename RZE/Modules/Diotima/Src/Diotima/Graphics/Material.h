#pragma once

#include <string>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>

#include <Utils/Interfaces/Resource.h>

namespace Diotima
{
	class GFXShaderGroup;
	class GFXTexture2D;

	class GFXMaterial : public IResource
	{
	public:
		GFXMaterial();
		~GFXMaterial();

		GFXShaderGroup* const GetShaderGroup() const;
		GFXTexture2D* const GetTexture2D() const;
		void SetShaderGroup(GFXShaderGroup* const shaderGroup);
		void SetTexture2D(GFXTexture2D* const texture2D);

	// IResource interface
	public:
		virtual bool Load(const std::string& filePath);

	public:
		void Use() const;

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
	};
}