#include <StdAfx.h>
#include <Graphics/ViewCapture.h>

#include <Graphics/RenderEngine.h>
#include <Graphics/TextureFile.h>

#include <Rendering/Graphics/RenderTarget.h>

ViewCapture::ViewCapture(U32 width, U32 height)
	: m_size(static_cast<float>(width), static_cast<float>(height))
{
	m_view.DrawsDebug = false;
}

ViewCapture::~ViewCapture()
{
}

std::shared_ptr<AsyncOperation> ViewCapture::Capture(RenderScene& scene, const RenderCamera& camera, const ViewCaptureSettings& settings, const Filepath& filepath)
{
	if (m_target == nullptr)
	{
		m_target = std::make_unique<Rendering::RenderTargetTexture>(static_cast<U32>(m_size.X()), static_cast<U32>(m_size.Y()));
		m_target->Initialize();
		m_view.Target = m_target.get();
	}

	m_view.Camera = camera;
	m_view.Camera.Viewport = RenderViewport{ m_size };
	m_view.ViewportSize = m_size;
	m_view.Lighting = settings.Lighting;
	m_view.AutoExposure = settings.AutoExposure;
	m_view.ExposureCompensation = settings.ExposureCompensation;

	RZE().GetRenderEngine().RenderSecondaryView("View Capture", m_view, scene);

	return TextureFile::SaveToPNG(m_target->GetTargetPlatformObject(), filepath);
}
