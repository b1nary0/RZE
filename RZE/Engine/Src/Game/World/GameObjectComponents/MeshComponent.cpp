#include <StdAfx.h>
#include <Game/World/GameObjectComponents/MeshComponent.h>

#include <Game/Model3D.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Game/World/GameObject/GameObject.h>

#include <Utils/DebugUtils/Debug.h>

#include <Graphics/RenderEngine.h>

void MeshComponent::SetMeshResource(const ResourceHandle& resource)
{
	m_resource = resource;
}

void MeshComponent::CreateRenderObject()
{
	ResourceHandler& resourceHandler = RZE::GetResourceHandler();
	RenderEngine& renderEngine = RZE::GetRenderEngine();

	// #TODO We should probably make MeshComponent a TransformComponent since a mesh without a transform is useless.
	const TransformComponent* const transformComponent = GetOwner()->GetComponent<TransformComponent>();
	AssertMsg(transformComponent != nullptr, "No TransformComponent found. Mesh creation is useless without a location in 3D space.");

	const Model3D* modelData = resourceHandler.GetResource<Model3D>(m_resource);
	AssertNotNull(modelData);

	m_renderProxy = renderEngine.CreateRenderProxy(modelData->GetStaticMesh().GetSubMeshes(), transformComponent->GetAsMat4x4());
}

void MeshComponent::OnAddToScene()
{
	CreateRenderObject();
}

void MeshComponent::Update()
{
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
