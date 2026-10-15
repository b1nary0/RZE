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
			return mesh != nullptr ? MakeObjectRefs(mesh->GetMesh().GetSubMeshes()) : std::vector<ObjectRef>();
		});
}

const Filepath& RenderComponent::GetMeshPath() const
{
	static const Filepath s_noMesh;
	return m_meshInstance != nullptr ? m_meshInstance->GetResourcePath() : s_noMesh;
}

void RenderComponent::LoadMesh(const Filepath& path)
{
	if (path.GetRelativePath() == GetMeshPath().GetRelativePath())
	{
		return;
	}

	std::shared_ptr<StaticMeshInstance> meshInstance;
	if (path.IsValid())
	{
		const ResourceHandle meshResource = RZE().GetResourceHandler().LoadResource<StaticMeshResource>(path);
		meshInstance = StaticMeshResource::CreateInstance(meshResource);
	}

	SetMeshInstance(std::move(meshInstance));
}

void RenderComponent::SetMeshInstance(std::shared_ptr<StaticMeshInstance> meshInstance)
{
	m_meshInstance = std::move(meshInstance);

	if (m_renderObject != nullptr)
	{
		if (m_meshInstance != nullptr)
		{
			m_renderObject->SetMeshInstance(m_meshInstance);
		}
		else
		{
			m_renderScene->DestroyRenderObject(m_renderObject);
		}
		GetOwner()->MarkHierarchyBoundsChanged();
		return;
	}

	// A component not yet in a scene creates its render object in OnAddToScene
	if (m_owner != nullptr && m_owner->IsInScene())
	{
		CreateRenderObject();
	}
}

Vector3D RenderComponent::GetMeshSize() const
{
	if (m_meshInstance == nullptr || m_owner == nullptr)
	{
		return Vector3D();
	}

	return m_meshInstance->GetMesh().GetBounds().GetSize() * m_owner->GetTransformComponent()->GetScale();
}

void RenderComponent::SetDrawMeshBounds(bool draw)
{
	m_drawMeshBounds = draw;
}

BoundingBox RenderComponent::GetWorldBounds() const
{
	return m_renderObject != nullptr ? m_renderObject->GetBounds() : BoundingBox();
}

BoundingBox RenderComponent::GetLocalBounds() const
{
	return m_renderObject != nullptr ? m_renderObject->GetLocalBounds() : BoundingBox();
}

void RenderComponent::CreateRenderObject()
{
	// Added without a mesh (e.g. via the editor); a render object is created once a mesh is selected
	if (m_meshInstance == nullptr)
	{
		return;
	}

	if (m_renderObject == nullptr)
	{
		// #TODO We should probably make RenderComponent a TransformComponent since a mesh without a transform is useless.
		GameObjectComponentPtr<TransformComponent> transformComponent = GetOwner()->GetComponent<TransformComponent>();
		AssertMsg(transformComponent != nullptr, "No TransformComponent found. Mesh creation is useless without a location in 3D space.");

		m_renderObject = m_renderScene->CreateRenderObject(m_meshInstance);
		m_renderObject->SetTransform(transformComponent->GetWorldMatrix());
		// Offset by one because IDs start at 0, which picking uses for "nothing"
		m_renderObject->SetPickID(GetOwner()->GetID() + 1);
		GetOwner()->MarkHierarchyBoundsChanged();

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
	GetOwner()->MarkHierarchyBoundsChanged();
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

	if (m_drawMeshBounds)
	{
		m_renderScene->DrawWireBox(m_meshInstance->GetMesh().GetBounds(), worldMatrix, k_meshBoundsColour);
	}

	if (m_drawSubMeshBounds)
	{
		for (const MeshDraw& draw : m_renderObject->GetDraws())
		{
			m_renderScene->DrawWireBox(draw.Geometry->GetBounds(), *draw.Transform, k_subMeshBoundsColour);
		}
	}
}

void RenderComponent::Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
{
	writer.String("RenderComponent");
	writer.StartObject();
	{
		writer.Key("ResourcePath");
		writer.String(GetMeshPath().GetRelativePath().c_str());
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

