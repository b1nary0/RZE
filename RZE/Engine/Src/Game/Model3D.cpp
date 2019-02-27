#include <StdAfx.h>
#include <Game/Model3D.h>

#include <Assimp/Importer.hpp>
#include <Assimp/postprocess.h>
#include <Assimp/scene.h>

#include <Graphics/Material.h>
#include <Graphics/Texture2D.h>

#include <Graphics/IndexBuffer.h>
#include <Graphics/VertexBuffer.h>

Model3D::Model3D()
{
}

Model3D::~Model3D()
{
}

bool Model3D::Load(const FilePath& filePath)
{
	BROFILER_EVENT("Model3D::Load");
	mFilePath = filePath;

	Assimp::Importer ModelImporter;
	const aiScene* AssimpScene = ModelImporter.ReadFile(mFilePath.GetAbsolutePath(),
		aiProcessPreset_TargetRealtime_Fast | aiProcess_ConvertToLeftHanded ^ aiProcess_FlipWindingOrder | aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph);

	bool bAssimpNotLoaded =
		!AssimpScene
		|| !AssimpScene->mRootNode
		|| AssimpScene->mFlags == AI_SCENE_FLAGS_INCOMPLETE;

	if (bAssimpNotLoaded)
	{
		// #TODO More informative error message.
		LOG_CONSOLE_ARGS("Failed to load model from [%s].", mFilePath.GetRelativePath().c_str());
		return false;
	}

	std::vector<MeshGeometry> meshGeometry;
	meshGeometry.reserve(AssimpScene->mNumMeshes);
	ProcessNode(*AssimpScene->mRootNode, *AssimpScene, meshGeometry);

	if (meshGeometry.size() != AssimpScene->mNumMeshes)
	{
		// #TODO More informative error message.
		LOG_CONSOLE("Error reading meshes.");
		return false;
	}

	mMesh.Initialize(meshGeometry);
	return true;
}

void Model3D::Release()
{
 	mTextureHandles.clear();
}

void Model3D::ProcessNode(const aiNode& node, const aiScene& scene, std::vector<MeshGeometry>& outMeshGeometry)
{
	for (U32 meshIndex = 0; meshIndex < node.mNumMeshes; meshIndex++)
	{
		const unsigned int assimpMeshIdx = node.mMeshes[meshIndex];
		const aiMesh& assimpMesh = *scene.mMeshes[assimpMeshIdx];

		outMeshGeometry.emplace_back();
		ProcessMesh(assimpMesh, scene, outMeshGeometry.back());
	}

	for (U32 childNodeIdx = 0; childNodeIdx < node.mNumChildren; childNodeIdx++)
	{
		ProcessNode(*node.mChildren[childNodeIdx], scene, outMeshGeometry);
	}
}

void Model3D::ProcessMesh(const aiMesh& mesh, const aiScene& scene, MeshGeometry& outMesh)
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
			Vector2D vertUV(assimpUV.x, -assimpUV.y);
			vertex.UVData = vertUV;
		}

		if (bHasTangents)
		{
			const aiVector3D& assimpTangent = mesh.mTangents[vertexIdx];
			Vector3D vertTangent(assimpTangent.x, assimpTangent.y, assimpTangent.z);
			vertex.Tangent = vertTangent;
		}

		outMesh.AddVertex(vertex);
	}

	for (U32 faceIdx = 0; faceIdx < mesh.mNumFaces; faceIdx++)
	{
		const aiFace& assimpFace = mesh.mFaces[faceIdx];
		for (U32 indexIdx = 0; indexIdx < assimpFace.mNumIndices; indexIdx++)
		{
			outMesh.AddIndex(assimpFace.mIndices[indexIdx]);
		}
	}
	// #TODO(Josh::Not necessarily just here, but definitely need to have a layer between renderer and engine to represent these concepts
		//             Essentially anything prefixed GFX should be renderer-only in the long run. Should get to this while Diotima scope is small)
	Material* pMaterial = new Material();
	if (mesh.mMaterialIndex >= 0)
	{
		aiMaterial* mat = scene.mMaterials[mesh.mMaterialIndex];

		float shininess = 0.0f;
		if (mat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
		{
			pMaterial->Shininess = shininess;
		}

		float opacity = 0.0f;
		if (mat->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS)
		{
			pMaterial->Opacity = opacity;
		}

		for (unsigned int i = 0; i < mat->GetTextureCount(aiTextureType_DIFFUSE); ++i)
		{
			aiString str;
			mat->GetTexture(aiTextureType_DIFFUSE, i, &str);

			FilePath texturePath = GetTextureFilePath(str.C_Str());
			ResourceHandle textureHandle = RZE_Application::RZE().GetResourceHandler().RequestResource<Texture2D>(texturePath, ETextureType::Diffuse);
			if (textureHandle.IsValid())
			{
				Texture2D* texture = RZE_Application::RZE().GetResourceHandler().GetResource<Texture2D>(textureHandle);
				pMaterial->SetDiffuse(texture);

				mTextureHandles.emplace_back(textureHandle);
			}
			else
			{
				LOG_CONSOLE_ARGS("Could not load texture at [%s]", texturePath.GetRelativePath().c_str());
			}
		}

		for (unsigned int i = 0; i < mat->GetTextureCount(aiTextureType_SPECULAR); ++i)
		{
			aiString str;
			mat->GetTexture(aiTextureType_SPECULAR, i, &str);

			FilePath texturePath = GetTextureFilePath(str.C_Str());
			ResourceHandle textureHandle = RZE_Application::RZE().GetResourceHandler().RequestResource<Texture2D>(texturePath, ETextureType::Specular);
			if (textureHandle.IsValid())
			{
				Texture2D* texture = RZE_Application::RZE().GetResourceHandler().GetResource<Texture2D>(textureHandle);
				pMaterial->SetSpecular(texture);

				mTextureHandles.emplace_back(textureHandle);
			}
			else
			{
				LOG_CONSOLE_ARGS("Could not load texture at [%s]", texturePath.GetRelativePath().c_str());
			}
		}

		for (unsigned int i = 0; i < mat->GetTextureCount(aiTextureType_NORMALS); ++i)
		{
			aiString str;
			mat->GetTexture(aiTextureType_NORMALS, i, &str);

			FilePath texturePath = GetTextureFilePath(str.C_Str());
			ResourceHandle textureHandle = RZE_Application::RZE().GetResourceHandler().RequestResource<Texture2D>(texturePath, ETextureType::Normal);
			if (textureHandle.IsValid())
			{
				Texture2D* texture = RZE_Application::RZE().GetResourceHandler().GetResource<Texture2D>(textureHandle);
				pMaterial->SetNormal(texture);

				mTextureHandles.emplace_back(textureHandle);
			}
			else
			{
				LOG_CONSOLE_ARGS("Could not load texture at [%s]", texturePath.GetRelativePath().c_str());
			}
		}

	}

	if (!pMaterial->IsTextured())
	{
		LOG_CONSOLE_ARGS("Could not find texture for [%s] loading default material", mFilePath.GetRelativePath().c_str());

		ResourceHandle diffuseHandle = RZE_Application::RZE().GetResourceHandler().RequestResource<Texture2D>(Texture2D::kDefaultDiffuseTexturePath, ETextureType::Diffuse);
		// #TODO(Josh::Potential bug here -- what happens if we then reconcile no texture at runtime? We would have to remove this from this list -- linear searches are bad
		mTextureHandles.push_back(diffuseHandle);

		pMaterial->SetDiffuse(RZE_Application::RZE().GetResourceHandler().GetResource<Texture2D>(diffuseHandle));
	}

	outMesh.SetMaterial(pMaterial);
	outMesh.AllocateGPUData();
}

// #TODO(Josh::Pull this out into a util function)
FilePath Model3D::GetTextureFilePath(const std::string& fileName)
{
	// #TODO(Josh::We need to bake the texture folder structure here, or get on proper asset referencing system)
	std::string path = mFilePath.GetRelativeDirectoryPath();
	path.append(fileName);

	return FilePath(path);
}
