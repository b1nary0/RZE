#include <StdAfx.h>
#include <Game/World/GameObjectComponents/RenderComponent.h>

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

void RenderComponent::SetMeshResource(const ResourceHandle& resource)
{
	m_resource = resource;
}

void RenderComponent::CreateRenderObject()
{
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
		m_renderObject->SetTransform(transformComponent->GetAsMat4x4());

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
		m_renderObject->SetTransform(transformComponent->GetAsMat4x4());
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

void RenderComponent::Deserialize(const rapidjson::Value& data)
{
	Filepath resourcePath = Filepath(data["ResourcePath"].GetString());

	m_resource = RZE().GetResourceHandler().LoadResource<StaticMeshResource>(resourcePath);
}

void RenderComponent::OnEditorInspect()
{
	ResourceHandler& resourceHandler = RZE().GetResourceHandler();

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
		FilePromptParams openFileParams =
		{
			"RZE MeshAsset",
			k_meshAssetDirectoryPath,
			"*.meshasset"
		};

		std::string chosenPath;
		bool openSuccess = RZE().ShowOpenFilePrompt(openFileParams, chosenPath);
		if (openSuccess)
		{
			Filepath path = Filepath::FromAbsolutePathStr(chosenPath);
			if (m_resource.IsValid())
			{
				resourceHandler.ReleaseResource(m_resource);
			}
			
			m_resource = resourceHandler.LoadResource<StaticMeshResource>(path);

			if (m_renderObject != nullptr)
			{
				RZE().GetRenderEngine().DestroyRenderObject(m_renderObject);
			}
			else
			{
				// @TODO meaningful error feedback - no crashy gamey tho
				// this means something went wrong during load of the resource
			}

			CreateRenderObject();
		}
	}
}
