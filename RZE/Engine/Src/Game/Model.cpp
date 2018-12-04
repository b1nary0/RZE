#include <StdAfx.h>
#include <Game/Model.h>

#include <Assimp/Importer.hpp>
#include <Assimp/postprocess.h>
#include <Assimp/scene.h>

#include <Diotima/RenderBatch.h>
#include <Diotima/Graphics/Mesh.h>
#include <Diotima/Graphics/Texture2D.h>

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
		aiProcess_Triangulate
		| aiProcess_GenNormals);

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

	ProcessNode(*AssimpScene->mRootNode, *AssimpScene);

	if (mMeshList.size() != AssimpScene->mNumMeshes)
	{
		// #TODO More informative error message.
		LOG_CONSOLE("Error reading meshes.");
		return false;
	}

	// #TODO(Josh::Request batch from renderer for now to keep it tracked in renderer and leased out.
	//		 Eventually want to move to have another layer in between engine -> renderer classes)
	mRenderBatch = std::make_unique<Diotima::RenderBatch>();
	mRenderBatch->Allocate(mMeshList);

	return true;
}

void Model3D::Release()
{
	for (size_t i = 0; i < mMeshList.size(); ++i)
	{
		delete mMeshList[i];
	}

 	mTextureHandles.clear();
}

Diotima::RenderBatch* Model3D::GetRenderBatch()
{
	return mRenderBatch.get();
}

void Model3D::ProcessNode(const aiNode& node, const aiScene& scene)
{
	for (U32 meshIdx = 0; meshIdx < node.mNumMeshes; meshIdx++)
	{
		const unsigned int assimpMeshIdx = node.mMeshes[meshIdx];
		const aiMesh& assimpMesh = *scene.mMeshes[assimpMeshIdx];

		Diotima::GFXMesh* Mesh = new Diotima::GFXMesh();
		ProcessMesh(assimpMesh, scene, *Mesh);
		mMeshList.push_back(Mesh);
	}

	for (U32 childNodeIdx = 0; childNodeIdx < node.mNumChildren; childNodeIdx++)
	{
		ProcessNode(*node.mChildren[childNodeIdx], scene);
	}
}

void Model3D::ProcessMesh(const aiMesh& mesh, const aiScene& scene, Diotima::GFXMesh& outMesh)
{
	bool bHasTextureCoords = mesh.mTextureCoords[0] != nullptr;
	for (U32 vertexIdx = 0; vertexIdx < mesh.mNumVertices; vertexIdx++)
	{
		const aiVector3D& assimpVert = mesh.mVertices[vertexIdx];
		const aiVector3D& assimpNormal = mesh.mNormals[vertexIdx];

		Vector3D vertPos(assimpVert.x, assimpVert.y, assimpVert.z);
		Vector3D vertNormal(assimpNormal.x, assimpNormal.y, assimpNormal.z);

		Diotima::GFXVertex vertex;
		vertex.Position = vertPos;
		vertex.Normal = vertNormal;

		if (bHasTextureCoords)
		{
			const aiVector3D& assimpUV = mesh.mTextureCoords[0][vertexIdx];
			Vector2D vertUV(assimpUV.x, assimpUV.y);
			vertex.UVData = vertUV;
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

	if (mesh.mMaterialIndex >= 0)
	{
		aiMaterial* mat = scene.mMaterials[mesh.mMaterialIndex];
		for (unsigned int i = 0; i < mat->GetTextureCount(aiTextureType_DIFFUSE); ++i)
		{
			aiString str;
			mat->GetTexture(aiTextureType_DIFFUSE, i, &str);

			FilePath texturePath = GetTextureFilePath(str.C_Str());
			ResourceHandle textureHandle = RZE_Application::RZE().GetResourceHandler().RequestResource<Diotima::GFXTexture2D>(texturePath, Diotima::ETextureType::Diffuse);
			if (textureHandle.IsValid())
			{
				outMesh.AddTexture(RZE_Application::RZE().GetResourceHandler().GetResource<Diotima::GFXTexture2D>(textureHandle));
				mTextureHandles.push_back(textureHandle);
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
			ResourceHandle textureHandle = RZE_Application::RZE().GetResourceHandler().RequestResource<Diotima::GFXTexture2D>(texturePath, Diotima::ETextureType::Specular);
			if (textureHandle.IsValid())
			{
				mTextureHandles.emplace_back(textureHandle);
				outMesh.AddTexture(RZE_Application::RZE().GetResourceHandler().GetResource<Diotima::GFXTexture2D>(textureHandle));
			}
			else
			{
				LOG_CONSOLE_ARGS("Could not load texture at [%s]", texturePath.GetRelativePath().c_str());
			}
		}
	}

	outMesh.OnLoadFinished();
}

// #TODO(Josh::Pull this out into a util function)
FilePath Model3D::GetTextureFilePath(const std::string& fileName)
{
	// #TODO(Josh::We need to bake the texture folder structure here, or get on proper asset referencing system)
	std::string path = mFilePath.GetRelativeDirectoryPath();
	path.append(fileName);

	return FilePath(path);
}
