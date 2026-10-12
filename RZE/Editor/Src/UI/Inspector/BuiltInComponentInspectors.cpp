// Custom inspector sections for the engine's components, for what their property descriptors can't show

#include <UI/Inspector/ComponentInspectors.h>
#include <UI/Inspector/MeshMaterialTree.h>

#include <Game/World/GameObjectComponents/CameraComponent.h>
#include <Game/World/GameObjectComponents/GifChatComponent.h>
#include <Game/World/GameObjectComponents/RenderComponent.h>

#include <Rendering/Graphics/RenderTarget.h>

#include <ImGui/imgui.h>

#include <algorithm>

namespace
{
	// What the camera sees, rendered into its preview target this frame
	void DrawCameraPreview(Editor::EditorContext& context, CameraComponent& camera)
	{
		camera.RenderPreview();

		const Rendering::RenderTargetTexture& target = camera.GetPreviewTarget();
		Rendering::TextureBuffer2DHandle texture = target.GetTargetPlatformObject();
		if (!texture.IsValid())
		{
			return;
		}

		// The preview only fills part of a larger target
		const Vector2D previewSize = CameraComponent::GetPreviewSize();
		const ImVec2 uvMax(
			std::min(1.0f, previewSize.X() / target.GetWidth()),
			std::min(1.0f, previewSize.Y() / target.GetHeight()));
		ImGui::Image((ImTextureID)(intptr_t)texture.GetTextureData(), ImVec2(previewSize.X(), previewSize.Y()), ImVec2(0.0f, 0.0f), uvMax);
	}

	void DrawRenderMeshTree(Editor::EditorContext& context, RenderComponent& renderComponent)
	{
		if (const StaticMeshInstance* const mesh = renderComponent.GetMesh())
		{
			Editor::DrawMeshMaterialTree(*mesh);
		}
	}

	void DrawGifChatMeshTree(Editor::EditorContext& context, GifChatComponent& gifChat)
	{
		Editor::DrawMeshMaterialTree(gifChat.GetMeshGeometry());
	}
}

REGISTER_COMPONENT_INSPECTOR(CameraComponent, DrawCameraPreview);
REGISTER_COMPONENT_INSPECTOR(RenderComponent, DrawRenderMeshTree);
REGISTER_COMPONENT_INSPECTOR(GifChatComponent, DrawGifChatMeshTree);
