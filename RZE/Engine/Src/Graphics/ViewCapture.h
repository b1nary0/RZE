#pragma once

#include <Graphics/RenderView.h>

#include <Utils/Math/Vector2D.h>
#include <Utils/PrimitiveDefs.h>

#include <memory>

class AsyncOperation;
class Filepath;
class RenderScene;

namespace Rendering
{
	class RenderTargetTexture;
}

// How a capture is lit and exposed; see RenderView's fields of the same names
struct ViewCaptureSettings
{
	ERenderViewLighting Lighting = ERenderViewLighting::Scene;
	bool AutoExposure = true;
	float ExposureCompensation = 0.0f;
};

// Renders a scene from a camera into an offscreen target of its own and saves it to an image file, e.g. for
// thumbnails or screenshots. Debug lines (the editor's grid, camera frustums, bounds boxes) aren't drawn.
class ViewCapture
{
public:
	// The size of every capture, in pixels
	ViewCapture(U32 width, U32 height);
	~ViewCapture();

	ViewCapture(const ViewCapture&) = delete;
	ViewCapture& operator=(const ViewCapture&) = delete;

public:
	const Vector2D& GetSize() const { return m_size; }

	// Renders scene from camera, which should be set up for GetSize()'s aspect ratio, as part of the current frame, and
	// saves it as a PNG through TextureFile::SaveToPNG. The image is read back from the commands queued now, so
	// another Capture can follow straight away.
	std::shared_ptr<AsyncOperation> Capture(RenderScene& scene, const RenderCamera& camera, const ViewCaptureSettings& settings, const Filepath& filepath);

private:
	Vector2D m_size;
	// Made on the first capture, since a capture can be set up before the renderer is
	std::unique_ptr<Rendering::RenderTargetTexture> m_target;
	RenderView m_view { ERenderViewKind::Secondary };
};
