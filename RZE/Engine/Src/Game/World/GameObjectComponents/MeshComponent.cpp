#include <StdAfx.h>
#include <Game/World/GameObjectComponents/MeshComponent.h>

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
		default:
			return "None";
		}
	}
}

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

	const StaticMeshResource* modelData = resourceHandler.GetResource<StaticMeshResource>(m_resource);
	AssertNotNull(modelData);

	m_renderObject = RZE::GetRenderEngine().CreateRenderObject(modelData->GetStaticMesh());
	m_renderObject->SetTransform(transformComponent->GetAsMat4x4());
}

void MeshComponent::OnAddToScene()
{
	CreateRenderObject();
}

void MeshComponent::OnRemoveFromScene()
{
	RZE::GetRenderEngine().DestroyRenderObject(m_renderObject);
}

void MeshComponent::Update()
{
	if (m_renderObject != nullptr)
	{
		const TransformComponent* const transformComponent = GetOwner()->GetComponent<TransformComponent>();
		AssertMsg(transformComponent != nullptr, "No TransformComponent found. Mesh creation is useless without a location in 3D space.");

		// @TODO Look into ways to avoid this
		m_renderObject->SetTransform(transformComponent->GetAsMat4x4());
	}
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

	m_resource = RZE::GetResourceHandler().LoadResource<StaticMeshResource>(resourcePath);
}

void MeshComponent::OnEditorInspect()
{
	ResourceHandler& resourceHandler = RZE::GetResourceHandler();

	if (m_resource.IsValid())
	{
		ImGui::Text(m_resource.GetResourcePath().GetRelativePath().c_str());

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

					ImGui::TextColored(ImVec4(0.65f, 0.65f, 1.0f, 1.0f), "[Shader] ");
					ImGui::Text(shader->GetName().c_str());

					ImGui::Separator();

					//
					// Textures
					//
					ImGui::TextColored(ImVec4(0.65f, 0.65f, 1.0f, 1.0f), "[Textures]");
					for (U8 textureSlot = 0; textureSlot < MaterialInstance::TEXTURE_SLOT_COUNT; ++textureSlot)
					{
						ResourceHandle textureResource = material->GetTexture(textureSlot);
						
						if (textureResource.IsValid())
						{
							const Texture2D* const textureData = resourceHandler.GetResource<Texture2D>(textureResource);
							AssertNotNull(textureData);

							const std::string textureTypeStr = GetTextureTypeStr(static_cast<MaterialInstance::TextureSlot>(textureSlot));
							if (ImGui::CollapsingHeader(textureTypeStr.c_str()))
							{
								ImGui::Text(textureData->GetFilepath().GetRelativePath().c_str());
								ImGui::Image(textureData->GetPlatformObject().GetTextureData(), ImVec2(256.0f, 256.0f));
							}
						}
					}
					ImGui::TreePop();
				}

				ImGui::TreePop();
			}
		}
	}

	ImGui::NewLine();
	if (ImGui::Button("Select Mesh..."))
	{
		FilePath path = RZE_Application::RZE().ShowOpenFilePrompt();
		if (path.IsValid())
		{

			if (m_resource.IsValid())
			{
				resourceHandler.ReleaseResource(m_resource);
			}
			
			m_resource = resourceHandler.LoadResource<StaticMeshResource>(path);

			if (m_renderObject != nullptr)
			{
				RZE::GetRenderEngine().DestroyRenderObject(m_renderObject);
			}

			CreateRenderObject();
		}
	}
}
