#include <Diotima/Graphics/Material.h>

#include <Diotima/Driver/OpenGL.h>
#include <Diotima/Graphics/Texture2D.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Platform/File.h>

//#include <RapidJSON/document.h>

namespace Diotima
{
	Diotima::GFXMaterial::GFXMaterial()
	{
	}

	Diotima::GFXMaterial::~GFXMaterial()
	{
	}

	bool Diotima::GFXMaterial::Load(const FilePath& filePath)
	{
		// 	File materialFile(filePath);
		// 
		// 	rapidjson::Document json;
		// 	json.Parse(materialFile.Content().c_str());

		return true;
	}
}
