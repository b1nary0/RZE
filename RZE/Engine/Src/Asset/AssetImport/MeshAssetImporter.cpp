#include <StdAfx.h>

#include <Asset/AssetImport/MeshAssetImporter.h>
#include <Asset/AssetImport/MeshAssetWriter.h>
#include <Asset/AssetImport/MaterialAssetImporter.h>

#include <Graphics/Material.h>
#include <Graphics/Shader.h>

#include <Utils/Memory/ByteStream.h>
#include <Utils/Memory/ByteStreamUtils.h>

namespace
{
	constexpr uint16_t k_meshImporterVersion = 0;
}

bool MeshAssetImporter::Import(const Filepath& filePath)
{
	AssertExpr(filePath.Exists());

	ByteStream byteStream(filePath.GetRelativePath());
	byteStream.ReadFromFile(filePath);

	MeshAssetFileHeader* headerData = reinterpret_cast<MeshAssetFileHeader*>(byteStream.PeekBytesAdvance(sizeof(MeshAssetFileHeader)));
	AssertExpr(headerData->AssetVersion == k_meshImporterVersion);

	for (int i = 0; i < headerData->MeshCount; ++i)
	{
		std::string meshName = ByteStreamUtils::ReadString(byteStream);
		std::string materialPath = ByteStreamUtils::ReadString(byteStream);
		std::vector<float> vertexData = ByteStreamUtils::ReadArray<float>(byteStream);
		std::vector<U32> indexData = ByteStreamUtils::ReadArray<U32>(byteStream);

		MeshVertex* vertexDataArray = reinterpret_cast<MeshVertex*>(vertexData.data());

		const size_t meshVertexBufferSize = vertexData.size() / (sizeof(MeshVertex) / 4);
		std::vector<MeshVertex> meshVertexArray;
		meshVertexArray.reserve(meshVertexBufferSize);
		meshVertexArray.insert(meshVertexArray.end(), &vertexDataArray[0], &vertexDataArray[meshVertexBufferSize]);

		MeshGeometry geo;
		geo.SetName(meshName);
		geo.SetVertexData(meshVertexArray);
		geo.SetIndexData(indexData);

		MaterialAssetImporter materialImporter;
		materialImporter.Import(Filepath(materialPath));	

		// #TODO
		// Not a huge fan of doing this material work here but for the moment it's really the only place that 
		// makes sense since we are constructing the MeshGeometry here. Ideally we don't do any resource loading
		// in the AssetImporter pipeline, it just spits out the requisite data to load in say MeshResource or any other
		// IResource derived object with a ::Load() function on it
		MaterialAssetImporter::MaterialData materialData = materialImporter.GetMaterialData();
		std::shared_ptr<MaterialInstance> materialInstance = MaterialInstance::Create(materialData);

		geo.SetMaterial(materialInstance);

		geo.AllocateData();

		mMeshGeometry.push_back(geo);
	}

	return true;
}
