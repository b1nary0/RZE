#include <StdAfx.h>
#include <Graphics/StaticMeshInstance.h>

#include <Utils/DebugUtils/Debug.h>

#include <GLM/common.hpp>

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

	glm::vec3 boundsMin = m_subMeshes[0].GetBoundsMin().GetInternalVec();
	glm::vec3 boundsMax = m_subMeshes[0].GetBoundsMax().GetInternalVec();
	for (const MeshGeometry& subMesh : m_subMeshes)
	{
		boundsMin = glm::min(boundsMin, subMesh.GetBoundsMin().GetInternalVec());
		boundsMax = glm::max(boundsMax, subMesh.GetBoundsMax().GetInternalVec());
	}

	m_boundsMin = Vector3D(boundsMin.x, boundsMin.y, boundsMin.z);
	m_boundsMax = Vector3D(boundsMax.x, boundsMax.y, boundsMax.z);
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
