#include <StdAfx.h>
#include <Game/World/GameObjectComponents/MeshComponent.h>

#include <Game/Model3D.h>

void MeshComponent::SetMeshResource(const ResourceHandle& resource)
{
	m_resource = resource;
}

void MeshComponent::Save(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
{
	writer.String("MeshComponent");
	writer.StartObject();
	{
		writer.Key("ResourcePath");
		writer.String(m_resource.GetResourcePath().GetRelativePath().c_str());
	}
	writer.EndObject();
}

void MeshComponent::Load(const rapidjson::Value& data)
{
	FilePath resourcePath = FilePath(data["ResourcePath"].GetString());

	m_resource = RZE::GetResourceHandler().LoadResource<Model3D>(resourcePath);
}
