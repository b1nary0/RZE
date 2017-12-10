#pragma once

namespace Diotima
{
	class GFXShaderGroup;

	class GFXMaterial
	{
	public:
		GFXMaterial();
		~GFXMaterial();

	public:
		void Use();

	private:
		GFXShaderGroup* mShaderGroup;
	};
}