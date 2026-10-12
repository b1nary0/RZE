#include <UI/Inspector/MeshMaterialTree.h>

#include <RZE.h>

#include <Graphics/Material.h>
#include <Graphics/Shader.h>
#include <Graphics/StaticMeshInstance.h>
#include <Graphics/Texture2D.h>

#include <ImGui/imgui.h>

#include <algorithm>

namespace
{
	const ImVec4 k_headingColour(0.65f, 0.65f, 1.0f, 1.0f);
	// Texture previews are scaled down to fit in this many pixels, keeping their aspect ratio
	constexpr float k_maxTexturePreviewSize = 256.0f;

	const char* GetTextureSlotName(MaterialInstance::TextureSlot textureSlot)
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

	void DrawTexture(const Texture2D& texture)
	{
		ImGui::TextUnformatted(texture.GetFilepath().GetRelativePath().c_str());

		const Vector2D& dimensions = texture.GetDimensions();
		const float scale = std::min(1.0f, k_maxTexturePreviewSize / std::max(dimensions.X(), dimensions.Y()));
		ImGui::Image((ImTextureID)(intptr_t)texture.GetPlatformObject().GetTextureData(), ImVec2(dimensions.X() * scale, dimensions.Y() * scale));
	}

	void DrawMaterial(const MaterialInstance& material)
	{
		ResourceHandler& resourceHandler = RZE().GetResourceHandler();

		if (!ImGui::TreeNode("##material", "Material: %s", material.GetName().c_str()))
		{
			return;
		}

		ImGui::Separator();

		ImGui::TextColored(k_headingColour, "[Shader]");
		const PixelShader* const shader = resourceHandler.GetResource<PixelShader>(material.GetShaderResource());
		ImGui::TextUnformatted(shader != nullptr ? shader->GetName().c_str() : "(none)");

		ImGui::NewLine();

		ImGui::TextColored(k_headingColour, "[Textures]");
		for (U8 textureSlot = 0; textureSlot < MaterialInstance::TEXTURE_SLOT_COUNT; ++textureSlot)
		{
			const ResourceHandle& textureResource = material.GetTexture(textureSlot);
			if (!textureResource.IsValid())
			{
				continue;
			}

			const Texture2D* const texture = resourceHandler.GetResource<Texture2D>(textureResource);
			if (texture == nullptr)
			{
				continue;
			}

			ImGui::PushID(textureSlot);
			if (ImGui::CollapsingHeader(GetTextureSlotName(static_cast<MaterialInstance::TextureSlot>(textureSlot))))
			{
				DrawTexture(*texture);
			}
			ImGui::PopID();
		}

		ImGui::TreePop();
	}
}

namespace Editor
{
	void DrawMeshMaterialTree(const StaticMeshInstance& mesh)
	{
		const std::vector<MeshGeometry>& subMeshes = mesh.GetSubMeshes();
		for (size_t subMeshIndex = 0; subMeshIndex < subMeshes.size(); ++subMeshIndex)
		{
			const MeshGeometry& subMesh = subMeshes[subMeshIndex];

			ImGui::PushID(static_cast<int>(subMeshIndex));
			if (ImGui::TreeNode("##subMesh", "%s", subMesh.GetName().c_str()))
			{
				const std::shared_ptr<const MaterialInstance> material = subMesh.GetMaterial();
				if (material != nullptr)
				{
					DrawMaterial(*material);
				}

				ImGui::TreePop();
			}
			ImGui::PopID();
		}
	}
}
