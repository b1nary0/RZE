#include <StdAfx.h>
#include <Game/World/GameObjectComponents/RenderComponent.h>

#include <EngineCore/Resources/ResourceDependencyList.h>

#include <Game/StaticMeshResource.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Game/World/GameObject/GameObject.h>

#include <Graphics/Material.h>
#include <Graphics/RenderEngine.h>
#include <Graphics/Shader.h>

#include <Utils/DebugUtils/Debug.h>

#include "Graphics/Texture2D.h"

namespace
{
	const Filepath k_meshAssetDirectoryPath("ProjectData/Mesh");

	// Distinct so both are readable when drawn together
	const Vector3D k_meshBoundsColour(1.0f, 0.85f, 0.0f);
	const Vector3D k_subMeshBoundsColour(0.0f, 0.8f, 1.0f);

	std::string GetTextureTypeStr(MaterialInstance::TextureSlot textureSlot)
	{
		switch (textureSlot)
		{
		case MaterialInstance::TEXTURE_SLOT_DIFFUSE:
				return "Diffuse";
		case MaterialInstance::TEXTURE_SLOT_SPECULAR:
				return "Specular";
		case MaterialInstance::TEXTURE_SLOT_NORMAL:
				return "Normal";
		case MaterialInstance::TEXTURE_SLOT_OPACITY:
				return "Opacity";
		case MaterialInstance::TEXTURE_SLOT_HEIGHT:
				return "Height";
		default:
			return "None";
		}
	}
}

void RenderComponent::DescribeProperties(PropertyBuilder<RenderComponent>& properties)
{
	properties.AssetPath("Mesh", &RenderComponent::GetMeshPath, &RenderComponent::LoadMesh)
		.Asset("RZE MeshAsset", k_meshAssetDirectoryPath.GetRelativePath().c_str(), "*.meshasset");
	properties.Vector3("Size", &RenderComponent::GetMeshSize)
		.DisplayFormat("%.2f m").Tip("Along the object's own axes, including its scale");
	properties.Bool("Draw Mesh Bounds", &RenderComponent::IsDrawingMeshBounds, &RenderComponent::SetDrawMeshBounds);
	properties.Bool("Draw Sub-Mesh Bounds", &RenderComponent::IsDrawingSubMeshBounds, &RenderComponent::SetDrawSubMeshBounds);
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
		RZE().GetRenderEngine().DestroyRenderObject(m_renderObject);
	}

	// Assigning releases the old mesh
	m_resource = path.IsValid() ? RZE().GetResourceHandler().LoadResource<StaticMeshResource>(path) : ResourceHandle::EmptyHandle();

	// A component not yet in a scene creates its render object in OnAddToScene
	if (m_owner != nullptr && m_owner->IsInScene())
	{
		CreateRenderObject();
	}
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
		RenderEngine& renderEngine = RZE().GetRenderEngine();

		// #TODO We should probably make RenderComponent a TransformComponent since a mesh without a transform is useless.
		GameObjectComponentPtr<TransformComponent> transformComponent = GetOwner()->GetComponent<TransformComponent>();
		AssertMsg(transformComponent != nullptr, "No TransformComponent found. Mesh creation is useless without a location in 3D space.");

		const StaticMeshResource* modelData = resourceHandler.GetResource<StaticMeshResource>(m_resource);
		AssertNotNull(modelData);

		m_renderObject = renderEngine.CreateRenderObject(modelData->GetStaticMesh());
		m_renderObject->SetTransform(transformComponent->GetWorldMatrix());
		// Offset by one because IDs start at 0, which picking uses for "nothing"
		m_renderObject->SetPickID(GetOwner()->GetID() + 1);

		return;
	}

	RZE_LOG("RenderComponent already has a RenderObject when calling CreateRenderObject()");
}

void RenderComponent::OnAddToScene()
{
	CreateRenderObject();
}

void RenderComponent::OnRemoveFromScene()
{
	RZE().GetRenderEngine().DestroyRenderObject(m_renderObject);
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
	RenderEngine& renderEngine = RZE().GetRenderEngine();
	const Matrix4x4& worldMatrix = m_renderObject->GetTransform();
	const StaticMeshInstance& staticMesh = m_renderObject->GetStaticMesh();

	if (m_drawMeshBounds)
	{
		renderEngine.DrawWireBox(staticMesh.GetBoundsMin(), staticMesh.GetBoundsMax(), worldMatrix, k_meshBoundsColour);
	}

	if (m_drawSubMeshBounds)
	{
		for (const MeshGeometry& subMesh : staticMesh.GetSubMeshes())
		{
			renderEngine.DrawWireBox(subMesh.GetBoundsMin(), subMesh.GetBoundsMax(), worldMatrix, k_subMeshBoundsColour);
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

// The fields are drawn from DescribeProperties; this is only the sub-mesh/material/texture tree
void RenderComponent::OnEditorInspect()
{
	ResourceHandler& resourceHandler = RZE().GetResourceHandler();

	if (m_resource.IsValid())
	{
		const StaticMeshResource* const modelData = resourceHandler.GetResource<StaticMeshResource>(m_resource);

		for (const auto& subMesh : modelData->GetStaticMesh().GetSubMeshes())
		{
			if (ImGui::TreeNode(&subMesh, subMesh.GetName().c_str()))
			{
				const std::shared_ptr<const MaterialInstance>& material = subMesh.GetMaterial();
				//
				// Material Information
				//
				if (ImGui::TreeNode(&material, "%s %s", "Material:", material->GetName().c_str()))
				{
					const PixelShader* const shader = resourceHandler.GetResource<PixelShader>(material->GetShaderResource());
					AssertNotNull(shader);
					
					ImGui::Separator();

					ImGui::TextColored(ImVec4(0.65f, 0.65f, 1.0f, 1.0f), "[Shader] ");
					ImGui::Text(shader->GetName().c_str());
					
					ImGui::NewLine();

					//
					// Textures
					//
					ImGui::TextColored(ImVec4(0.65f, 0.65f, 1.0f, 1.0f), "[Textures]");
					for (U8 textureSlot = 0; textureSlot < MaterialInstance::TEXTURE_SLOT_COUNT; ++textureSlot)
					{
						const ResourceHandle& textureResource = material->GetTexture(textureSlot);
						
						if (textureResource.IsValid())
						{
							const Texture2D* const textureData = resourceHandler.GetResource<Texture2D>(textureResource);
							AssertNotNull(textureData);

							const std::string textureTypeStr = GetTextureTypeStr(static_cast<MaterialInstance::TextureSlot>(textureSlot));
							if (ImGui::CollapsingHeader(textureTypeStr.c_str()))
							{
								ImGui::Text(textureData->GetFilepath().GetRelativePath().c_str());
								ImGui::Image((ImTextureID)(intptr_t)textureData->GetPlatformObject().GetTextureData(), ImVec2(256.0f, 256.0f));
							}
						}
					}
					ImGui::TreePop();
				}

				ImGui::TreePop();
			}
		}
	}
}
