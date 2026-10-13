#include <StdAfx.h>
#include <Game/World/GameObjectComponents/RenderComponent.h>

#include <Asset/AssetPaths.h>

#include <EngineCore/Resources/ResourceDependencyList.h>

#include <Game/GameScene.h>
#include <Game/StaticMeshResource.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Game/World/GameObject/GameObject.h>

#include <Graphics/Material.h>
#include <Graphics/RenderScene.h>
#include <Graphics/Shader.h>

#include <Utils/DebugUtils/Debug.h>

#include "Graphics/Texture2D.h"

namespace
{
	// Distinct so both are readable when drawn together
	const Vector3D k_meshBoundsColour(1.0f, 0.85f, 0.0f);
	const Vector3D k_subMeshBoundsColour(0.0f, 0.8f, 1.0f);
}

void RenderComponent::DescribeProperties(PropertyBuilder<RenderComponent>& properties)
{
	properties.AssetPath("Mesh", &RenderComponent::GetMeshPath, &RenderComponent::LoadMesh)
		.Asset("RZE MeshAsset", AssetPaths::GetMeshFolder().GetRelativePath().c_str(), (std::string("*") + AssetPaths::k_meshExtension).c_str());
	properties.Vector3("Size", &RenderComponent::GetMeshSize)
		.DisplayFormat("%.2f m").ToolTip("Along the object's own axes, including its scale");
	properties.Bool("Draw Mesh Bounds", &RenderComponent::IsDrawingMeshBounds, &RenderComponent::SetDrawMeshBounds).EditorOnly();
	properties.Bool("Draw Sub-Mesh Bounds", &RenderComponent::IsDrawingSubMeshBounds, &RenderComponent::SetDrawSubMeshBounds).EditorOnly();
	properties.Array("Sub-Meshes", [](const RenderComponent& renderComponent)
		{
			const StaticMeshInstance* const mesh = renderComponent.GetMesh();
			return mesh != nullptr ? MakeObjectRefs(mesh->GetSubMeshes()) : std::vector<ObjectRef>();
		});
}

const Filepath& RenderComponent::GetMeshPath() const
{
	static const Filepath s_noMesh;
	return m_resource.IsValid() ? m_resource.GetResourcePath() : s_noMesh;
}

void RenderComponent::LoadMesh(const Filepath& path)
{
	if (path.GetRelativePath() == GetMeshPath().GetRelativePath())
	{
		return;
	}

	// The old render object goes before the old mesh is released, since it was built from it
	if (m_renderObject != nullptr)
	{
		m_renderScene->DestroyRenderObject(m_renderObject);
	}

	// Assigning releases the old mesh
	m_resource = path.IsValid() ? RZE().GetResourceHandler().LoadResource<StaticMeshResource>(path) : ResourceHandle::EmptyHandle();

	// A component not yet in a scene creates its render object in OnAddToScene
	if (m_owner != nullptr && m_owner->IsInScene())
	{
		CreateRenderObject();
	}
}

const StaticMeshInstance* RenderComponent::GetMesh() const
{
	if (!m_resource.IsValid())
	{
		return nullptr;
	}

	const StaticMeshResource* const modelData = RZE().GetResourceHandler().GetResource<StaticMeshResource>(m_resource);
	return modelData != nullptr ? &modelData->GetStaticMesh() : nullptr;
}

Vector3D RenderComponent::GetMeshSize() const
{
	if (!m_resource.IsValid() || m_owner == nullptr)
	{
		return Vector3D();
	}

	const StaticMeshResource* const modelData = RZE().GetResourceHandler().GetResource<StaticMeshResource>(m_resource);
	if (modelData == nullptr)
	{
		return Vector3D();
	}

	const StaticMeshInstance& staticMesh = modelData->GetStaticMesh();
	return (staticMesh.GetBoundsMax() - staticMesh.GetBoundsMin()) * m_owner->GetTransformComponent()->GetScale();
}

void RenderComponent::SetDrawMeshBounds(bool draw)
{
	m_drawMeshBounds = draw;
}

bool RenderComponent::GetWorldBounds(Vector3D& outMin, Vector3D& outMax)
{
	if (m_renderObject == nullptr)
	{
		return false;
	}

	outMin = m_renderObject->GetBoundsMin();
	outMax = m_renderObject->GetBoundsMax();
	return true;
}

void RenderComponent::CreateRenderObject()
{
	// Added without a mesh (e.g. via the editor); a render object is created once a mesh is selected
	if (!m_resource.IsValid())
	{
		return;
	}

	if (m_renderObject == nullptr)
	{
		ResourceHandler& resourceHandler = RZE().GetResourceHandler();

		// #TODO We should probably make RenderComponent a TransformComponent since a mesh without a transform is useless.
		GameObjectComponentPtr<TransformComponent> transformComponent = GetOwner()->GetComponent<TransformComponent>();
		AssertMsg(transformComponent != nullptr, "No TransformComponent found. Mesh creation is useless without a location in 3D space.");

		const StaticMeshResource* modelData = resourceHandler.GetResource<StaticMeshResource>(m_resource);
		AssertNotNull(modelData);

		m_renderObject = m_renderScene->CreateRenderObject(modelData->GetStaticMesh());
		m_renderObject->SetTransform(transformComponent->GetWorldMatrix());
		// Offset by one because IDs start at 0, which picking uses for "nothing"
		m_renderObject->SetPickID(GetOwner()->GetID() + 1);

		return;
	}

	RZE_LOG("RenderComponent already has a RenderObject when calling CreateRenderObject()");
}

void RenderComponent::Initialize()
{
	m_renderScene = &GetOwner()->GetScene().GetRenderScene();
}

void RenderComponent::OnAddToScene()
{
	CreateRenderObject();
}

void RenderComponent::OnRemoveFromScene()
{
	m_renderScene->DestroyRenderObject(m_renderObject);
}

void RenderComponent::Update()
{
	if (m_renderObject != nullptr)
	{
		GameObjectComponentPtr<TransformComponent> transformComponent = GetOwner()->GetComponent<TransformComponent>();
		AssertMsg(transformComponent != nullptr, "No TransformComponent found. Mesh creation is useless without a location in 3D space.");

		// @TODO Look into ways to avoid this
		m_renderObject->SetTransform(transformComponent->GetWorldMatrix());

		if (m_drawMeshBounds || m_drawSubMeshBounds)
		{
			DrawBounds();
		}
	}
}

void RenderComponent::DrawBounds()
{
	const Matrix4x4& worldMatrix = m_renderObject->GetTransform();
	const StaticMeshInstance& staticMesh = m_renderObject->GetStaticMesh();

	if (m_drawMeshBounds)
	{
		m_renderScene->DrawWireBox(staticMesh.GetBoundsMin(), staticMesh.GetBoundsMax(), worldMatrix, k_meshBoundsColour);
	}

	if (m_drawSubMeshBounds)
	{
		for (const MeshGeometry& subMesh : staticMesh.GetSubMeshes())
		{
			m_renderScene->DrawWireBox(subMesh.GetBoundsMin(), subMesh.GetBoundsMax(), worldMatrix, k_subMeshBoundsColour);
		}
	}
}

void RenderComponent::Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
{
	writer.String("RenderComponent");
	writer.StartObject();
	{
		writer.Key("ResourcePath");
		writer.String(m_resource.GetResourcePath().GetRelativePath().c_str());
	}
	writer.EndObject();
}

void RenderComponent::GatherResourceDependencies(const rapidjson::Value& componentData, ResourceDependencyList& outDependencies)
{
	rapidjson::Value::ConstMemberIterator resourcePath = componentData.FindMember("ResourcePath");
	if (resourcePath != componentData.MemberEnd() && resourcePath->value.IsString())
	{
		outDependencies.Add<StaticMeshResource>(resourcePath->value.GetString());
	}
}

void RenderComponent::Deserialize(const rapidjson::Value& data)
{
	// Preloaded by GatherResourceDependencies during an async scene load, so this is normally a cache hit
	LoadMesh(Filepath(data["ResourcePath"].GetString()));
}

