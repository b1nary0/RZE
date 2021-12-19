#include <SourceAssetBurner/Importers/AssimpSourceImporter.h>

#include <Assimp/Importer.hpp>
#include <Assimp/postprocess.h>
#include <Assimp/scene.h>

#include <Utils/Memory/ByteStream.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/PrimitiveDefs.h>

#include "Utils/Platform/File.h"

#include <filesystem>

namespace
{
	// #TODO
	// Move these into a common file. They will be used across different classes
	constexpr char kMeshAssetSuffix[] = { ".meshasset" };
	constexpr char kMaterialAssetSuffix[] = { ".materialasset" };

	constexpr int kMeshAssetVersion = 1;

	std::string StripAssetNameFromFilePath(const FilePath& filePath)
	{
		std::string assetPath = filePath.GetRelativePath();

		size_t pos = assetPath.find_last_of('/');
		std::string assetName = assetPath.substr(pos + 1, assetPath.size());
		assetName = assetName.substr(0, assetName.find_last_of('.'));

		return assetName;
	}
}

AssimpSourceImporter::AssimpSourceImporter()
{
}

bool AssimpSourceImporter::Import(const FilePath& filePath)
{
	LOG_CONSOLE_ARGS("AssimpSourceImporter : Importing %s...", filePath.GetRelativePath().c_str());

	mAssetName = StripAssetNameFromFilePath(filePath);

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
		LOG_CONSOLE_ARGS("Failed to load model from [%s].", filePath.GetRelativePath().c_str());
		return false;
	}

	mMeshes.reserve(AssimpScene->mNumMeshes);
	ProcessNode(*AssimpScene->mRootNode, *AssimpScene);

	if (mMeshes.size() != AssimpScene->mNumMeshes)
	{
		LOG_CONSOLE_ARGS("Error reading meshes from [%s].", filePath.GetRelativePath().c_str());
		return false;
	}

	if (!WriteMeshAsset())
	{
		LOG_CONSOLE_ARGS("Failed to write meshasset for [%s]. See logs.", filePath.GetRelativePath().c_str());
		return false;
	}

	if (!WriteMaterialAsset())
	{
		LOG_CONSOLE_ARGS("Failed to write materialAsset for [%s]. See logs.", filePath.GetRelativePath().c_str());
		return false;
	}

	LOG_CONSOLE_ARGS("AssimpSourceImporter : Import Success for %s", filePath.GetRelativePath().c_str());
	return true;
}

void AssimpSourceImporter::ProcessNode(const aiNode& node, const aiScene& scene)
{
	for (U32 meshIndex = 0; meshIndex < node.mNumMeshes; meshIndex++)
	{
		const unsigned int assimpMeshIdx = node.mMeshes[meshIndex];
		const aiMesh& assimpMesh = *scene.mMeshes[assimpMeshIdx];

		mMeshes.emplace_back();
		MeshData& meshData = mMeshes.back();

		if (assimpMesh.mName.length > 0)
		{
			meshData.MeshName = assimpMesh.mName.C_Str();
		}
		else
		{
			meshData.MeshName = node.mName.C_Str();
		}

		LOG_CONSOLE_ARGS("AssimpSourceImporter : Processing Mesh %i of %i - %s", meshIndex + 1, node.mNumMeshes, meshData.MeshName.c_str());
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

	//bool bHasDiffuse = false;
	//bool bHasSpecular = false;
	//bool bHasBump = false;

	//Material* pMaterial;
	if (mesh.mMaterialIndex >= 0)
	{
		MaterialData materialData;

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

		if (!mMaterialTable.count(materialData.MaterialName))
		{
			mMaterialTable[materialData.MaterialName] = std::move(materialData);
		}
	}

	//	for (unsigned int i = 0; i < mat->GetTextureCount(aiTextureType_DIFFUSE); ++i)
	//	{
	//		aiString str;
	//		mat->GetTexture(aiTextureType_DIFFUSE, i, &str);

	//		FilePath texturePath = GetTextureFilePath(str.C_Str());

	//		// #TODO
	//		// Should really shorthand these long lines by doing the following at the top somewhere for all the callsites to use
	//		// ResourceHandler& resourceHandler = RZE::GetResourceHandler();
	//		// resourceHandler.LoadResource<...>
	//		// but should do it as a larger effort instead of sprinkled efforts
	//		ResourceHandle textureHandle = RZE::GetResourceHandler().LoadResource<Texture2D>(texturePath, Texture2D::ETextureType::Diffuse);
	//		if (textureHandle.IsValid())
	//		{
	//			pMaterial->SetTexture(Material::TEXTURE_SLOT_DIFFUSE, textureHandle);
	//			bHasDiffuse = true;
	//		}
	//		else
	//		{
	//			LOG_CONSOLE_ARGS("Could not load texture at [%s]", texturePath.GetRelativePath().c_str());
	//		}
	//	}

	//	for (unsigned int i = 0; i < mat->GetTextureCount(aiTextureType_SPECULAR); ++i)
	//	{
	//		aiString str;
	//		mat->GetTexture(aiTextureType_SPECULAR, i, &str);

	//		FilePath texturePath = GetTextureFilePath(str.C_Str());
	//		ResourceHandle textureHandle = RZE::GetResourceHandler().LoadResource<Texture2D>(texturePath, Texture2D::ETextureType::Specular);
	//		if (textureHandle.IsValid())
	//		{
	//			pMaterial->SetTexture(Material::TEXTURE_SLOT_SPECULAR, textureHandle);
	//			bHasSpecular = true;
	//		}
	//		else
	//		{
	//			LOG_CONSOLE_ARGS("Could not load texture at [%s]", texturePath.GetRelativePath().c_str());
	//		}
	//	}

	//	for (unsigned int i = 0; i < mat->GetTextureCount(aiTextureType_NORMALS); ++i)
	//	{
	//		aiString str;
	//		mat->GetTexture(aiTextureType_NORMALS, i, &str);

	//		FilePath texturePath = GetTextureFilePath(str.C_Str());
	//		ResourceHandle textureHandle = RZE::GetResourceHandler().LoadResource<Texture2D>(texturePath, Texture2D::ETextureType::Normal);
	//		if (textureHandle.IsValid())
	//		{
	//			pMaterial->SetTexture(Material::TEXTURE_SLOT_NORMAL, textureHandle);
	//			bHasBump = true;
	//		}
	//		else
	//		{
	//			LOG_CONSOLE_ARGS("Could not load texture at [%s]", texturePath.GetRelativePath().c_str());
	//		}
	//	}
	//}

	//// #TODO
	//// This is essentially stubbing code. More to prove out the support infrastructure.
	////AssertMsg(bHasDiffuse, "Unsupported without diffuse texture for the hopefully short time being");
	//const bool bFullShading = bHasDiffuse && bHasSpecular && bHasBump;

	//ResourceHandle shaderTechnique;
	//if (bFullShading)
	//{
	//	shaderTechnique = RZE::GetResourceHandler().LoadResource<ShaderTechnique>(FilePath("Assets/Shaders/Pixel_NewRenderer.hlsl"), "Pixel_NewRenderer");
	//}
	//else if (bHasDiffuse)
	//{
	//	shaderTechnique = RZE::GetResourceHandler().LoadResource<ShaderTechnique>(FilePath("Assets/Shaders/Pixel_NewRenderer_DiffuseOnly.hlsl"), "Pixel_NewRenderer_DiffuseOnly");
	//}

	//// #TODO
	//// If we have no diffuse/bump/specular use the default asf shader.
	//// This is an atrocious way to handle this but in lieu of a better system and more patience
	//// it will have to do for now. Lots of bugs here, not really thought out code.
	//if (!shaderTechnique.IsValid() && !bHasSpecular)
	//{
	//	shaderTechnique = RZE::GetResourceHandler().LoadResource<ShaderTechnique>(FilePath("Assets/Shaders/Pixel_Default_NewRenderer.hlsl"), "Pixel_Default_NewRenderer");
	//}

	//pMaterial->SetShaderTechnique(shaderTechnique);
	//outMesh.SetMaterial(pMaterial);
	//// #TODO
	//// Instead of doing this, and having redundant memory on MeshGeometry
	//// we can instead form the combined data buffer here and set it on the mesh directly
	//// then if we need a higher level view of the data we can create that when needed?
	//outMesh.AllocateData();
}

bool AssimpSourceImporter::WriteMeshAsset()
{
	AssertExpr(mMeshes.size() > 0);

	// #TODO really gross string manip lol
	std::string assetFilename = mAssetName + kMeshAssetSuffix;
	std::string assetFilepathRelative = "ProjectData/Mesh/";
	assetFilepathRelative = assetFilepathRelative + assetFilename;

	FilePath outputPath(assetFilepathRelative);
	if (!std::filesystem::exists(outputPath.GetAbsolutePath()))
	{
		std::filesystem::create_directories(outputPath.GetAbsoluteDirectoryPath());
	}

	// #TODO json files are too large. need to compress and write the stream - see zlib
	File outputFile(outputPath);
	outputFile.Open((File::EFileOpenMode::Value)(File::EFileOpenMode::Write | File::EFileOpenMode::Binary));

	size_t bufSize = 0;
	for (auto& mesh : mMeshes)
	{
		// #TODO
		// Am i just dumb or can this be done in a much more elegant manner by a monkey
		const size_t meshDataSize = mesh.MeshName.size() + sizeof(MeshVertex) * mesh.VertexDataArray.size() + sizeof(U32) * mesh.IndexArray.size();
		bufSize += meshDataSize;
	}
	// magic numbers atm re: calculating data size headers per piece of data:
	// bufSize + meshCount + nameSizeBytes + vertexDataSizeBytes + indexDataSizeBytes
	bufSize += sizeof(size_t) * 2 + (sizeof(size_t) * 3 * mMeshes.size());
	const size_t meshCount = mMeshes.size();

	ByteStream byteStream(outputPath.GetRelativePath(), bufSize);

	byteStream.WriteBytes(&bufSize, sizeof(size_t));
	byteStream.WriteBytes(&meshCount, sizeof(size_t));

	for (auto& mesh : mMeshes)
	{
		// Have to piece-meal the copy here because the vectors are dynamically allocated. 
		const size_t nameSizeBytes = mesh.MeshName.size();
		byteStream.WriteBytes(&nameSizeBytes, sizeof(size_t));
		byteStream.WriteBytes(mesh.MeshName.data(), nameSizeBytes);

		const size_t vertexDataSizeBytes = sizeof(MeshVertex) * mesh.VertexDataArray.size();
		byteStream.WriteBytes(&vertexDataSizeBytes, sizeof(size_t));
		byteStream.WriteBytes(mesh.VertexDataArray.data(), vertexDataSizeBytes);

		const size_t indexDataSizeBytes = sizeof(U32) * mesh.IndexArray.size();
		byteStream.WriteBytes(&indexDataSizeBytes, sizeof(size_t));
		byteStream.WriteBytes(mesh.IndexArray.data(), indexDataSizeBytes);
	}

	AssertExpr(byteStream.GetNumBytesWritten() == bufSize);

	Byte* bytes = byteStream.GetBytes();
	for (size_t byteIndex = 0; byteIndex < byteStream.GetNumBytesWritten(); ++byteIndex)
	{
		outputFile << bytes[byteIndex];
	}

	outputFile.Close();

	LOG_CONSOLE_ARGS("AssimpSourceImporter : %s written.", outputPath.GetRelativePath().c_str());

	return true;
}

bool AssimpSourceImporter::WriteMaterialAsset()
{
	// #TODO really gross string manip lol
	std::string assetFilename = mAssetName + kMaterialAssetSuffix;
	std::string assetFilepathRelative = "ProjectData/Material/";
	assetFilepathRelative = assetFilepathRelative + assetFilename;

	FilePath outputPath(assetFilepathRelative);
	if (!std::filesystem::exists(outputPath.GetAbsolutePath()))
	{
		std::filesystem::create_directories(outputPath.GetAbsoluteDirectoryPath());
	}

	// #TODO json files are too large. need to compress and write the stream - see zlib
	File outputFile(outputPath);
	outputFile.Open((File::EFileOpenMode::Value)(File::EFileOpenMode::Write | File::EFileOpenMode::Binary));

	size_t bufSize = 0;
	size_t materialCount = mMaterialTable.size();
	for (auto& dataPair : mMaterialTable)
	{
		bufSize += dataPair.second.MaterialName.length();
	}
	bufSize += sizeof(size_t) * 2 + (sizeof(MaterialData::MaterialProperties) /*+ sizeof(U8)*/ * mMaterialTable.size());

	ByteStream byteStream(outputPath.GetRelativePath(), bufSize);

	byteStream.WriteBytes(&bufSize, sizeof(size_t));
	byteStream.WriteBytes(&materialCount, sizeof(size_t));
	for (auto& dataPair : mMaterialTable)
	{
		byteStream.WriteBytes(dataPair.second.MaterialName.data(), dataPair.second.MaterialName.length());
		byteStream.WriteBytes(&dataPair.second.Properties, sizeof(MaterialData::MaterialProperties));
	}

	AssertExpr(byteStream.GetNumBytesWritten() == bufSize);

	Byte* bytes = byteStream.GetBytes();
	for (size_t byteIndex = 0; byteIndex < byteStream.GetNumBytesWritten(); ++byteIndex)
	{
		outputFile << bytes[byteIndex];
	}

	outputFile.Close();

	LOG_CONSOLE_ARGS("AssimpSourceImporter : %s written.", outputPath.GetRelativePath().c_str());

	return true;
}

bool AssimpSourceImporter::WriteTextureAsset()
{
	return false;
}
