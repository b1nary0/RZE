#pragma once

#include <string>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>

#include <Utils/Interfaces/Resource.h>

namespace Diotima
{
	class GFXMaterial : public IResource
	{
	public:
		GFXMaterial();
		~GFXMaterial();

		// IResource interface
	public:
		virtual bool Load(const std::string& filePath);
		virtual void Release() {} // Nothing to release atm

	public:
		Vector4D Color;
	};
}