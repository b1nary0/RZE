#include <StdAfx.h>
#include <Game/World/GameObjectComponents/MeshComponent.h>

void MeshComponent::SetMeshResource(const ResourceHandle& resource)
{
	m_resource = resource;
}

void MeshComponent::Save(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
{
}

void MeshComponent::Load(const rapidjson::Value& data)
{
}
