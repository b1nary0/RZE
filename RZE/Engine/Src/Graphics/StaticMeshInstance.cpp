#include <StdAfx.h>
#include <Graphics/StaticMeshInstance.h>

#include <Utils/DebugUtils/Debug.h>

#include <Utils/Math/Math.h>

StaticMeshInstance::StaticMeshInstance()
{
}

StaticMeshInstance::~StaticMeshInstance()
{
}

void StaticMeshInstance::Initialize(const std::vector<MeshGeometry>& meshGeometry)
{
	m_subMeshes = meshGeometry;

	CalculateBounds();
}

void StaticMeshInstance::CalculateBounds()
{
	if (m_subMeshes.empty())
	{
		m_boundsMin = Vector3D();
		m_boundsMax = Vector3D();
		return;
	}

	m_boundsMin = m_subMeshes[0].GetBoundsMin();
	m_boundsMax = m_subMeshes[0].GetBoundsMax();
	for (const MeshGeometry& subMesh : m_subMeshes)
	{
		m_boundsMin = VectorUtils::Min(m_boundsMin, subMesh.GetBoundsMin());
		m_boundsMax = VectorUtils::Max(m_boundsMax, subMesh.GetBoundsMax());
	}
}

const std::vector<MeshGeometry>& StaticMeshInstance::GetSubMeshes() const
{
	return m_subMeshes;
}

const std::string& StaticMeshInstance::GetName() const
{
	return m_name;
}

void StaticMeshInstance::SetName(const std::string& name)
{
	AssertExpr(!name.empty());
	m_name = name;
}

std::vector<MeshGeometry>& StaticMeshInstance::GetSubMeshes()
{
	return m_subMeshes;
}
