// Custom inspector sections for the engine's components, for what their property descriptors can't show

#include <UI/Inspector/ComponentInspectors.h>
#include <UI/Inspector/PropertyDrawers.h>

#include <Game/World/GameObjectComponents/CameraComponent.h>

#include <Rendering/Graphics/RenderTarget.h>

#include <algorithm>

namespace
{
	// What the camera sees, rendered into its preview target this frame. A custom inspector rather than a property,
	// since showing it means rendering, not reading a value.
	void DrawCameraPreview(Editor::EditorContext& context, CameraComponent& camera)
	{
		camera.RenderPreview();

		const Rendering::RenderTargetTexture& target = camera.GetPreviewTarget();

		// The preview only fills part of a larger target
		PropertyTexture preview;
		preview.Handle = target.GetTargetPlatformObject();
		preview.Size = CameraComponent::GetPreviewSize();
		preview.UVMax = Vector2D(
			std::min(1.0f, preview.Size.X() / target.GetWidth()),
			std::min(1.0f, preview.Size.Y() / target.GetHeight()));

		// At its own size, which the render target was made for
		Editor::PropertyDrawers::DrawTexture(preview, 0.0f);
	}
}

REGISTER_COMPONENT_INSPECTOR(CameraComponent, DrawCameraPreview);
