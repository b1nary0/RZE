#include <StdAfx.h>
#include <Graphics/ReflectionProbeObject.h>

#include <Graphics/GraphicsDefines.h>

ReflectionProbeObject::ReflectionProbeObject(U64& sceneRevision)
	: m_sceneRevision(sceneRevision)
	, m_liveID(RenderSceneLifetime::Acquire())
{
	MarkChanged();
}

ReflectionProbeObject::~ReflectionProbeObject()
{
	RenderSceneLifetime::Release(m_liveID);
}

void ReflectionProbeObject::SetPosition(const Vector3D& position)
{
	if (position != m_position)
	{
		m_position = position;
		MarkChanged();
	}
}

void ReflectionProbeObject::SetExtents(const Vector3D& extents)
{
	if (extents != m_extents)
	{
		m_extents = extents;
		MarkChanged();
	}
}

void ReflectionProbeObject::MarkChanged()
{
	m_revision = ++m_sceneRevision;
}
