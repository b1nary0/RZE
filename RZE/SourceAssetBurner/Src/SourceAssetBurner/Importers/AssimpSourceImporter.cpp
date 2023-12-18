#include <SourceAssetBurner/Importers/AssimpSourceImporter.h>

#include <Assimp/Importer.hpp>
#include <Assimp/postprocess.h>
#include <Assimp/scene.h>

#include <Utils/Memory/ByteStream.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/PrimitiveDefs.h>

#include <Utils/Platform/File.h>

#include <filesystem>

namespace
{
	// #TODO
	// Move these into a common file. They will be used across different classes
	constexpr char kMaterialAssetSuffix[] = { ".materialasset" };
	
	std::string StripAssetNameFromFilePath(const Filepath& filePath)
	{
		const std::string& assetPath = filePath.GetRelativePath();

		size_t pos = assetPath.find_last_of('\\');
		std::string assetName = assetPath.substr(pos + 1, assetPath.size());
		assetName = assetName.substr(0, assetName.find_last_of('.'));

		return assetName;
	}

	Filepath GetTextureFilePath(const Filepath& filePath, const std::string& fileName)
	{
		// #TODO(Josh::We need to bake the texture folder structure here, or get on proper asset referencing system)
		std::string path = filePath.GetRelativeDirectoryPath();
		path.append(fileName);

		return Filepath(path);
	}
}

AssimpSourceImporter::AssimpSourceImporter()
{
}

bool AssimpSourceImporter::Import(const Filepath& filePath)
{
	RZE_LOG_ARGS("AssimpSourceImporter : Importing %s...", filePath.GetRelativePath().c_str());

	m_filepath = filePath;
	m_assetName = StripAssetNameFromFilePath(filePath);

	Assimp::Importer ModelImporter;
	const aiScene* AssimpScene = ModelImporter.ReadFile(filePath.GetAbsolutePath(),
		aiProcessPreset_TargetRealtime_Fast |
		// #TODO(Is this negation of aiProces_FlipWindingOrder even legal? does it have any consequences?)
		(aiProcess_ConvertToLeftHanded ^ aiProcess_FlipWindingOrder) |
		aiProcess_OptimizeMeshes |
		aiProcess_OptimizeGraph);

	bool bAssimpNotLoaded =
		!AssimpScene
		|| !AssimpScene->mRootNode
		|| AssimpScene->mFlags == AI_SCENE_FLAGS_INCOMPLETE;

	if (bAssimpNotLoaded)
	{
		// #TODO More informative error message.
		RZE_LOG_ARGS("Failed to load model from [%s].", filePath.GetRelativePath().c_str());
		return false;
	}

	m_meshes.reserve(AssimpScene->mNumMeshes);
	ProcessNode(*AssimpScene->mRootNode, *AssimpScene);

	if (m_meshes.size() != AssimpScene->mNumMeshes)
	{
		RZE_LOG_ARGS("Error reading meshes from [%s].", filePath.GetRelativePath().c_str());
		return false;
	}

	if (!WriteMeshAsset())
	{
		RZE_LOG_ARGS("Failed to write meshasset for [%s]. See logs.", filePath.GetRelativePath().c_str());
		return false;
	}

	for (auto& dataPair : m_materialTable)
	{
		if (!WriteMaterialAsset(dataPair.first, dataPair.second))
		{
			RZE_LOG_ARGS("Failed to write materialAsset for [%s]. See logs.", filePath.GetRelativePath().c_str());
			return false;
		}
	}

	RZE_LOG_ARGS("AssimpSourceImporter : Import Success for %s", filePath.GetRelativePath().c_str());
	return true;
}

void AssimpSourceImporter::ProcessNode(const aiNode& node, const aiScene& scene)
{
	for (U32 meshIndex = 0; meshIndex < node.mNumMeshes; meshIndex++)
	{
		const unsigned int assimpMeshIdx = node.mMeshes[meshIndex];
		const aiMesh& assimpMesh = *scene.mMeshes[assimpMeshIdx];
		
		MeshData& meshData = m_meshes.emplace_back();

		if (assimpMesh.mName.length > 0)
		{
			meshData.MeshName = assimpMesh.mName.C_Str();
		}
		else
		{
			meshData.MeshName = node.mName.C_Str();
		}

		RZE_LOG_ARGS("AssimpSourceImporter : Processing Mesh %i of %i - %s", meshIndex + 1, node.mNumMeshes, meshData.MeshName.c_str());
		ProcessMesh(assimpMesh, scene, meshData);
	}

	for (U32 childNodeIdx = 0; childNodeIdx < node.mNumChildren; childNodeIdx++)
	{
		ProcessNode(*node.mChildren[childNodeIdx], scene);
	}
}

void AssimpSourceImporter::ProcessMesh(const aiMesh& mesh, const aiScene& scene, MeshData& outMeshData)
{
	bool bHasTextureCoords = mesh.mTextureCoords[0] != nullptr;
	bool bHasTangents = mesh.mTangents != nullptr;

	for (U32 vertexIdx = 0; vertexIdx < mesh.mNumVertices; vertexIdx++)
	{
		const aiVector3D& assimpVert = mesh.mVertices[vertexIdx];
		const aiVector3D& assimpNormal = mesh.mNormals[vertexIdx];

		Vector3D vertPos(assimpVert.x, assimpVert.y, assimpVert.z);
		Vector3D vertNormal(assimpNormal.x, assimpNormal.y, assimpNormal.z);

		MeshVertex vertex;
		vertex.Position = vertPos;
		vertex.Normal = vertNormal;

		if (bHasTextureCoords)
		{
			const aiVector3D& assimpUV = mesh.mTextureCoords[0][vertexIdx];
			Vector2D vertUV(assimpUV.x, assimpUV.y);
			vertex.UVData = vertUV;
		}

		if (bHasTangents)
		{
			const aiVector3D& assimpTangent = mesh.mTangents[vertexIdx];
			Vector3D vertTangent(assimpTangent.x, assimpTangent.y, assimpTangent.z);
			vertex.Tangent = vertTangent;
		}

		outMeshData.VertexDataArray.emplace_back(vertex);
	}

	for (U32 faceIdx = 0; faceIdx < mesh.mNumFaces; faceIdx++)
	{
		const aiFace& assimpFace = mesh.mFaces[faceIdx];
		for (U32 indexIdx = 0; indexIdx < assimpFace.mNumIndices; indexIdx++)
		{
			outMeshData.IndexArray.push_back(assimpFace.mIndices[indexIdx]);
		}
	}

	if (mesh.mMaterialIndex >= 0)
	{
		// #TODO
		// This is just temp
		const int kTextureTypeCount = 3;

		MaterialData materialData;
		materialData.TexturePaths.resize(kTextureTypeCount);

		aiMaterial* mat = scene.mMaterials[mesh.mMaterialIndex];
		aiString matName;
		mat->Get(AI_MATKEY_NAME, matName);

		materialData.MaterialName = matName.C_Str();

		float shininess = 0.0f;
		if (mat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
		{
			materialData.Properties.Shininess = shininess;
		}

		float opacity = 0.0f;
		if (mat->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS)
		{
			materialData.Properties.Opacity = opacity;
		}

		aiColor3D diffColour;
		if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffColour) == AI_SUCCESS)
		{
		}

		// #TODO
		// Not entirely sure what would come in with > 0 of these texture types (though obviously its valid)
		// so in the interest of being explicit of what RZE supports at the moment, only take the 0th texture data
		if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0)
		{
			aiString str;
			mat->GetTexture(aiTextureType_DIFFUSE, 0, &str);

			Filepath texturePath = GetTextureFilePath(m_filepath, str.C_Str());

			materialData.TextureFlags |= MaterialData::ETextureFlags::TEXTUREFLAG_ALBEDO;
			materialData.TexturePaths[0] = texturePath.GetRelativePath();
		}

		if (mat->GetTextureCount(aiTextureType_SPECULAR) > 0)
		{
			aiString str;
			mat->GetTexture(aiTextureType_SPECULAR, 0, &str);

			Filepath texturePath = GetTextureFilePath(m_filepath, str.C_Str());

			materialData.TextureFlags |= MaterialData::ETextureFlags::TEXTUREFLAG_SPECULAR;
			materialData.TexturePaths[1] = texturePath.GetRelativePath();
		}

		if (mat->GetTextureCount(aiTextureType_NORMALS))
		{
			aiString str;
			mat->GetTexture(aiTextureType_NORMALS, 0, &str);

			Filepath texturePath = GetTextureFilePath(m_filepath, str.C_Str());

			materialData.TextureFlags |= MaterialData::ETextureFlags::TEXTUREFLAG_NORMAL;
			materialData.TexturePaths[2] = texturePath.GetRelativePath();
		}

		// #TODO really gross string manip lol
		std::string assetFilename = materialData.MaterialName + kMaterialAssetSuffix;
		std::string assetFilepathRelative = "ProjectData/Material/" + m_assetName + "/";
		assetFilepathRelative = assetFilepathRelative + assetFilename;

		outMeshData.MaterialPath = assetFilepathRelative;

		if (!m_materialTable.count(assetFilepathRelative))
		{
			m_materialTable[assetFilepathRelative] = std::move(materialData);
		}
	}
}

bool AssimpSourceImporter::WriteMeshAsset()
{
	MeshAssetWriter assetWriter;
	assetWriter.SetAssetName(m_assetName);
	assetWriter.SetMeshData(std::move(m_meshes));

	assetWriter.Write();

	return true;
}

bool AssimpSourceImporter::WriteMaterialAsset(const std::string& relativePath, const MaterialData& materialData)
{
	AssertExpr(!relativePath.empty());

	Filepath outputPath(relativePath);
	if (!outputPath.Exists())
	{
		Filepath::CreateDir(outputPath.GetAbsoluteDirectoryPath());
	}

	// #TODO json files are too large. need to compress and write the stream - see zlib
	File outputFile(outputPath);
	outputFile.Open((File::EFileOpenMode::Value)(File::EFileOpenMode::Write | File::EFileOpenMode::Binary));

	// sizeof(size_t) * 3 == bufSize + nameSizeBytes + textureCount
	size_t bufSize = (sizeof(size_t) * 3) + materialData.MaterialName.length() + sizeof(MaterialData::MaterialProperties) + sizeof(U8);
	for (const std::string& texturePath : materialData.TexturePaths)
	{
		// count the bytes of the texture string size in sizeof(size_t) here
		bufSize += sizeof(size_t) + texturePath.size();
	}

	ByteStream byteStream(outputPath.GetRelativePath(), bufSize);

	byteStream.WriteBytes(&bufSize, sizeof(size_t));

	const size_t nameSizeBytes = materialData.MaterialName.size();
	byteStream.WriteBytes(&nameSizeBytes, sizeof(size_t));
	byteStream.WriteBytes(materialData.MaterialName.data(), nameSizeBytes);

	byteStream.WriteBytes(&materialData.Properties, sizeof(MaterialData::MaterialProperties));
	byteStream.WriteBytes(&materialData.TextureFlags, sizeof(U8));

	const size_t kTextureCount = materialData.TexturePaths.size();
	byteStream.WriteBytes(&kTextureCount, sizeof(size_t));
	for (int textureIdx = 0; textureIdx < kTextureCount; ++textureIdx)
	{
		const std::string& texturePath = materialData.TexturePaths[textureIdx];

		const size_t pathSizeBytes = texturePath.size();
		byteStream.WriteBytes(&pathSizeBytes, sizeof(size_t));
		byteStream.WriteBytes(texturePath.data(), pathSizeBytes);
	}

	AssertExpr(byteStream.GetNumBytesWritten() == bufSize);

	const Byte* bytes = byteStream.GetBytes();
	for (size_t byteIndex = 0; byteIndex < byteStream.GetNumBytesWritten(); ++byteIndex)
	{
		outputFile << bytes[byteIndex];
	}

	outputFile.Close();

	RZE_LOG_ARGS("AssimpSourceImporter : %s written.", outputPath.GetRelativePath().c_str());

	return true;
}

bool AssimpSourceImporter::WriteTextureAsset()
{
	return false;
}
