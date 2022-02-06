#include <StdAfx.h>
#include <Game/Model3D.h>

#include <Graphics/Material.h>
#include <Graphics/Texture2D.h>

#include <Graphics/Shader.h>

#include <Asset/AssetImport/MeshAssetImporter.h>

Model3D::Model3D()
{
}

Model3D::~Model3D()
{
}

bool Model3D::Load(const FilePath& filePath)
{
	OPTICK_EVENT("Model3D::Load");
	MeshAssetImporter meshImporter;
	meshImporter.Import(filePath);

	mMesh.Initialize(std::move(meshImporter.GetMeshGeometry()));
	return true;
}

void Model3D::Release()
{
	for (auto& subMesh : mMesh.GetSubMeshes())
	{
		std::shared_ptr<Material> material = subMesh.GetMaterial();
		for (U8 textureSlot = 0; textureSlot < Material::TextureSlot::TEXTURE_SLOT_COUNT; ++textureSlot)
		{
			ResourceHandle resource = material->GetTexture(textureSlot);
			if (resource.IsValid())
			{
				RZE::GetResourceHandler().ReleaseResource(resource);
			}
		}
	}
}
