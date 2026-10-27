#include <StdAfx.h>
#include <Graphics/RenderObject.h>

#include <Graphics/GraphicsDefines.h>
#include <Graphics/RenderScene.h>

RenderObject::RenderObject()
	: m_liveID(RenderSceneLifetime::Acquire())
{
}

RenderObject::~RenderObject()
{
	RenderSceneLifetime::Release(m_liveID);
}

void RenderObject::SetMeshInstance(const std::shared_ptr<const StaticMeshInstance>& meshInstance)
{
	const BoundingBox oldBounds = GetBounds();
	m_meshInstance = meshInstance;
	const bool hasSubMeshes = m_meshInstance != nullptr && !m_meshInstance->GetMesh().GetSubMeshes().empty();
	m_localBounds = hasSubMeshes ? m_meshInstance->GetMesh().GetBounds() : BoundingBox();
	// The draws still point into the old mesh until GetDraws rebuilds them, which is the only way to read them
	MarkChanged(oldBounds);
}

void RenderObject::SetTransform(const Matrix4x4& transform)
{
	if (transform != m_matrices.Transform)
	{
		const BoundingBox oldBounds = GetBounds();
		m_matrices.Transform = transform;
		m_matrices.InvTransform = transform.Inverse();
		MarkChanged(oldBounds);
	}
}

void RenderObject::SetMobility(ERenderMobility mobility)
{
	if (mobility != m_mobility)
	{
		m_mobility = mobility;
		if (m_scene != nullptr)
		{
			m_scene->OnRenderObjectMobilityChanged(GetBounds());
		}
	}
}

const BoundingBox& RenderObject::GetBounds() const
{
	if (m_isBoundsStale)
	{
		m_bounds = m_localBounds.Transformed(m_matrices.Transform);
		m_isBoundsStale = false;
	}
	return m_bounds;
}

const std::vector<MeshDraw>& RenderObject::GetDraws() const
{
	if (m_areDrawsStale)
	{
		if (m_meshInstance != nullptr)
		{
			m_meshInstance->BuildDraws(m_matrices, m_draws);
		}
		else
		{
			m_draws.clear();
		}
		m_areDrawsStale = false;
	}
	return m_draws;
}

void RenderObject::MarkChanged(const BoundingBox& oldBounds)
{
	m_isBoundsStale = true;
	m_areDrawsStale = true;
	if (m_scene != nullptr)
	{
		m_scene->OnRenderObjectChanged(m_mobility, oldBounds, GetBounds());
	}
}
