#include <StdAfx.h>
#include <Game/Model3D.h>

#include <Assimp/Importer.hpp>
#include <Assimp/postprocess.h>
#include <Assimp/scene.h>

#include <Graphics/Material.h>
#include <Graphics/Texture2D.h>

#include <Graphics/Shader.h>

#include <AssetImport/MeshAssetImporter.h>

Model3D::Model3D()
{
}

Model3D::~Model3D()
{
}

bool Model3D::Load(const FilePath& filePath)
{
	OPTICK_EVENT("Model3D::Load");
	mFilePath = filePath;

	Assimp::Importer ModelImporter;
	const aiScene* AssimpScene = ModelImporter.ReadFile(mFilePath.GetAbsolutePath(),
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
		LOG_CONSOLE_ARGS("Failed to load model from [%s].", mFilePath.GetRelativePath().c_str());
		return false;
	}

	MeshAssetImporter meshImporter;
	meshImporter.Import(FilePath("ProjectData/Mesh/Nyra.meshasset"));

	std::vector<MeshGeometry> meshGeometry;
	meshGeometry.reserve(AssimpScene->mNumMeshes);
	ProcessNode(*AssimpScene->mRootNode, *AssimpScene, meshGeometry);

	if (meshGeometry.size() != AssimpScene->mNumMeshes)
	{
		LOG_CONSOLE_ARGS("Error reading meshes from [%s].", mFilePath.GetRelativePath().c_str());
		return false;
	}

	mMesh.Initialize(std::move(meshGeometry));
	return true;
}

void Model3D::Release()
{
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
			Vector2D vertUV(assimpUV.x, assimpUV.y);
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
	
	bool bHasDiffuse = false;
	bool bHasSpecular = false;
	bool bHasBump = false;

	Material* pMaterial;
	if (mesh.mMaterialIndex >= 0)
	{
		aiMaterial* mat = scene.mMaterials[mesh.mMaterialIndex];
		aiString matName;
		mat->Get(AI_MATKEY_NAME, matName);

		pMaterial = MaterialDatabase::Get().GetOrCreateMaterial(matName.C_Str());

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

			// #TODO
			// Should really shorthand these long lines by doing the following at the top somewhere for all the callsites to use
			// ResourceHandler& resourceHandler = RZE::GetResourceHandler();
			// resourceHandler.LoadResource<...>
			// but should do it as a larger effort instead of sprinkled efforts
			ResourceHandle textureHandle = RZE::GetResourceHandler().LoadResource<Texture2D>(texturePath, Texture2D::ETextureType::Diffuse);
			if (textureHandle.IsValid())
			{
				pMaterial->SetTexture(Material::TEXTURE_SLOT_DIFFUSE, textureHandle);
				bHasDiffuse = true;
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
			ResourceHandle textureHandle = RZE::GetResourceHandler().LoadResource<Texture2D>(texturePath, Texture2D::ETextureType::Specular);
			if (textureHandle.IsValid())
			{
				pMaterial->SetTexture(Material::TEXTURE_SLOT_SPECULAR, textureHandle);
				bHasSpecular = true;
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
			ResourceHandle textureHandle = RZE::GetResourceHandler().LoadResource<Texture2D>(texturePath, Texture2D::ETextureType::Normal);
			if (textureHandle.IsValid())
			{
				pMaterial->SetTexture(Material::TEXTURE_SLOT_NORMAL, textureHandle);
				bHasBump = true;
			}
			else
			{
				LOG_CONSOLE_ARGS("Could not load texture at [%s]", texturePath.GetRelativePath().c_str());
			}
		}
	}

	// #TODO
	// This is essentially stubbing code. More to prove out the support infrastructure.
	//AssertMsg(bHasDiffuse, "Unsupported without diffuse texture for the hopefully short time being");
	const bool bFullShading = bHasDiffuse && bHasSpecular && bHasBump;

	ResourceHandle shaderTechnique;
	if (bFullShading)
	{
		shaderTechnique = RZE::GetResourceHandler().LoadResource<ShaderTechnique>(FilePath("Assets/Shaders/Pixel_NewRenderer.hlsl"), "Pixel_NewRenderer");
	}
	else if (bHasDiffuse)
	{
		shaderTechnique = RZE::GetResourceHandler().LoadResource<ShaderTechnique>(FilePath("Assets/Shaders/Pixel_NewRenderer_DiffuseOnly.hlsl"), "Pixel_NewRenderer_DiffuseOnly");
	}

	// #TODO
	// If we have no diffuse/bump/specular use the default asf shader.
	// This is an atrocious way to handle this but in lieu of a better system and more patience
	// it will have to do for now. Lots of bugs here, not really thought out code.
	if (!shaderTechnique.IsValid() && !bHasSpecular)
	{
		shaderTechnique = RZE::GetResourceHandler().LoadResource<ShaderTechnique>(FilePath("Assets/Shaders/Pixel_Default_NewRenderer.hlsl"), "Pixel_Default_NewRenderer");
	}

	pMaterial->SetShaderTechnique(shaderTechnique);
	outMesh.SetMaterial(pMaterial);
	// #TODO
	// Instead of doing this, and having redundant memory on MeshGeometry
	// we can instead form the combined data buffer here and set it on the mesh directly
	// then if we need a higher level view of the data we can create that when needed?
	outMesh.AllocateData();
}

// #TODO(Josh::Pull this out into a util function)
FilePath Model3D::GetTextureFilePath(const std::string& fileName)
{
	// #TODO(Josh::We need to bake the texture folder structure here, or get on proper asset referencing system)
	std::string path = mFilePath.GetRelativeDirectoryPath();
	path.append(fileName);

	return FilePath(path);
}
