#include <StdAfx.h>

#include <Asset/AssetImport/MeshAssetImporter.h>
#include <Asset/AssetImport/MaterialAssetImporter.h>

#include <Graphics/Material.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>

#include <Utils/Memory/ByteStream.h>
#include <Utils/Memory/ByteStreamUtils.h>

namespace
{
	// #TODO
	// Read/Write this information to .meshasset
	constexpr int kAssetImportVersion = 1;
}

bool MeshAssetImporter::Import(const FilePath& filePath)
{
	ByteStream byteStream(filePath.GetRelativePath());
	byteStream.ReadFromFile(filePath);

	struct MeshAssetFileHeader
	{
		size_t BufSize;
		size_t MeshCount;
	};

	MeshAssetFileHeader* headerData = reinterpret_cast<MeshAssetFileHeader*>(byteStream.PeekBytesAdvance(sizeof(MeshAssetFileHeader)));
	for (int i = 0; i < headerData->MeshCount; ++i)
	{
		std::string meshName = ByteStreamUtils::ReadString(byteStream);
		std::string materialPath = ByteStreamUtils::ReadString(byteStream);
		std::vector<float> vertexData = ByteStreamUtils::ReadArray<float>(byteStream);
		std::vector<U32> indexData = ByteStreamUtils::ReadArray<U32>(byteStream);

		MeshVertex* vertexDataArray = reinterpret_cast<MeshVertex*>(vertexData.data());

		std::vector<MeshVertex> meshVertexArray;
		meshVertexArray.insert(meshVertexArray.end(), &vertexDataArray[0], &vertexDataArray[vertexData.size() / (sizeof(MeshVertex) / 4)]);

		MeshGeometry geo;
		geo.SetName(meshName);
		geo.SetVertexData(meshVertexArray);
		geo.SetIndexData(indexData);

		MaterialAssetImporter materialImporter;
		materialImporter.Import(FilePath(materialPath));	

		// #TODO
		// Not a huge fan of doing this material work here but for the moment it's really the only place that 
		// makes sense since we are constructing the MeshGeometry here. Ideally we don't do any resource loading
		// in the AssetImporter pipeline, it just spits out the requisite data to load in say Model3D or any other
		// IResource derived object with a ::Load() function on it
		MaterialAssetImporter::MaterialData materialData = materialImporter.GetMaterialData();
		std::shared_ptr<MaterialInstance> materialInstance = MaterialDatabase::Get().GetOrCreateMaterial(materialData.MaterialName);

		U8 textureSlot = 0; // #TODO This is a really hacky way to do this. Not safe at all in any other circumstance
		for (const std::string& texturePath : materialData.TexturePaths)
		{
			if (!texturePath.empty())
			{
				materialInstance->SetTexture(textureSlot++, RZE::GetResourceHandler().LoadResource<Texture2D>(FilePath(texturePath)));
			}
			else
			{
				textureSlot++;
			}
		}
		
		if ((materialData.TextureFlags & MaterialAssetImporter::MaterialData::TEXTUREFLAG_ALL) == MaterialAssetImporter::MaterialData::TEXTUREFLAG_ALL)
		{
			FilePath fullShaderPath("Assets/Shaders/Pixel_NewRenderer.hlsl");
			materialInstance->SetShaderTechnique(RZE::GetResourceHandler().LoadResource<PixelShader>(fullShaderPath, "Pixel_NewRenderer"));
		}
		else if (materialData.TextureFlags == MaterialAssetImporter::MaterialData::TEXTUREFLAG_NONE)
		{
			FilePath noTextureShaderPath("Assets/Shaders/Pixel_Default_NewRenderer.hlsl");
			materialInstance->SetShaderTechnique(RZE::GetResourceHandler().LoadResource<PixelShader>(noTextureShaderPath, "Pixel_Default_NewRenderer"));
		}
		else
		{
			FilePath diffuseOnlyPath("Assets/Shaders/Pixel_NewRenderer_DiffuseOnly.hlsl");
			materialInstance->SetShaderTechnique(RZE::GetResourceHandler().LoadResource<PixelShader>(diffuseOnlyPath, "Pixel_NewRenderer_DiffuseOnly"));
		}

		materialInstance->GetProperties().Shininess = materialData.Properties.Shininess;
		materialInstance->GetProperties().Opacity = materialData.Properties.Opacity;
		materialInstance->CommitPropertyChanges();

		geo.SetMaterial(materialInstance);

		geo.AllocateData();

		mMeshGeometry.push_back(geo);
	}

	return true;
}
