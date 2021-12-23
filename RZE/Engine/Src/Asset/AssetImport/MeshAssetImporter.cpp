#include <StdAfx.h>

#include <Asset/AssetImport/MeshAssetImporter.h>
#include <Asset/AssetImport/MaterialAssetImporter.h>

#include <Graphics/Material.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>

#include <Utils/Memory/ByteStream.h>

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
		std::string meshName = ReadName(byteStream);
		std::string materialPath = ReadName(byteStream);
		std::vector<float> vertexData = ReadVertices(byteStream);
		std::vector<U32> indexData = ReadIndices(byteStream);

		std::vector<MeshVertex> meshVertexArray;

		MeshVertex* vertexDataArray = reinterpret_cast<MeshVertex*>(vertexData.data());
		meshVertexArray.insert(meshVertexArray.end(), &vertexDataArray[0], &vertexDataArray[vertexData.size() / (sizeof(MeshVertex) / 4)]);

		MeshGeometry geo;
		geo.SetName(meshName);
		geo.SetVertexData(meshVertexArray);
		geo.SetIndexData(indexData);

		MaterialAssetImporter materialImporter;
		materialImporter.Import(FilePath(materialPath));	

		// #TODO
		// Not a huge fan of doing this material work here but for the moment it's really the only place that 
		// makes sense since we are constructing the MeshGeometry here.
		MaterialAssetImporter::MaterialData materialData = materialImporter.GetMaterialData();
		Material* material = MaterialDatabase::Get().GetOrCreateMaterial(materialData.MaterialName);
		material->Shininess = materialData.Properties.Shininess;
		material->Opacity = materialData.Properties.Opacity;

		U8 textureSlot = 0; // #TODO This is a really hacky way to do this. Not safe at all in any other circumstance
		for (const std::string& texturePath : materialData.TexturePaths)
		{
			if (!texturePath.empty())
			{
				material->SetTexture(textureSlot++, RZE::GetResourceHandler().LoadResource<Texture2D>(FilePath(texturePath)));
			}
			else
			{
				textureSlot++;
			}
		}

		if ((materialData.TextureFlags & MaterialAssetImporter::MaterialData::TEXTUREFLAG_ALL) == MaterialAssetImporter::MaterialData::TEXTUREFLAG_ALL)
		{
			FilePath fullShaderPath("Assets/Shaders/Pixel_NewRenderer.hlsl");
			material->SetShaderTechnique(RZE::GetResourceHandler().LoadResource<ShaderTechnique>(fullShaderPath, "Pixel_NewRenderer"));
		}
		else if (materialData.TextureFlags == MaterialAssetImporter::MaterialData::TEXTUREFLAG_NONE)
		{
			FilePath noTextureShaderPath("Assets/Shaders/Pixel_Default_NewRenderer.hlsl");
			material->SetShaderTechnique(RZE::GetResourceHandler().LoadResource<ShaderTechnique>(noTextureShaderPath, "Pixel_Default_NewRenderer"));
		}
		else
		{
			FilePath diffuseOnlyPath("Assets/Shaders/Pixel_NewRenderer_DiffuseOnly.hlsl");
			material->SetShaderTechnique(RZE::GetResourceHandler().LoadResource<ShaderTechnique>(diffuseOnlyPath, "Pixel_NewRenderer_DiffuseOnly"));
		}

		geo.SetMaterial(material);

		geo.AllocateData();

		mMeshGeometry.push_back(geo);
	}

	return true;
}

std::string MeshAssetImporter::ReadName(ByteStream& byteStream)
{
	size_t nameSizeBytes = *reinterpret_cast<size_t*>(byteStream.PeekBytesAdvance(sizeof(size_t)));

	unsigned char* name = new unsigned char[nameSizeBytes + 1];
	byteStream.ReadBytes(name, nameSizeBytes);
	name[nameSizeBytes] = '\0';
	std::string string((char*)name);

	delete[] name;
	name = nullptr;
	return std::move(string);
}

std::vector<float> MeshAssetImporter::ReadVertices(ByteStream& byteStream)
{
	size_t vertexDataSizeBytes = *reinterpret_cast<size_t*>(byteStream.PeekBytesAdvance(sizeof(size_t)));

	std::vector<float> vertexDataVec;
	vertexDataVec.resize(vertexDataSizeBytes / sizeof(float));
	byteStream.ReadBytes((Byte*)vertexDataVec.data(), vertexDataSizeBytes);

	return vertexDataVec;
}

std::vector<U32> MeshAssetImporter::ReadIndices(ByteStream& byteStream)
{
	size_t indexDataSizeBytes = *reinterpret_cast<size_t*>(byteStream.PeekBytesAdvance(sizeof(size_t)));

	std::vector<U32> indexDataVec;
	indexDataVec.resize(indexDataSizeBytes / sizeof(U32));
	byteStream.ReadBytes((Byte*)indexDataVec.data(), indexDataSizeBytes);

	return indexDataVec;
}
