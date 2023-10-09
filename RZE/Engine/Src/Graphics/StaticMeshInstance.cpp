#include <StdAfx.h>
#include <Graphics/StaticMeshInstance.h>

StaticMeshInstance::StaticMeshInstance()
{
}

StaticMeshInstance::~StaticMeshInstance()
{
}

void StaticMeshInstance::Initialize(const std::vector<MeshGeometry>& meshGeometry)
{
	m_subMeshes = meshGeometry;
}

const std::vector<MeshGeometry>& StaticMeshInstance::GetSubMeshes() const
{
	return m_subMeshes;
}

std::vector<MeshGeometry>& StaticMeshInstance::GetSubMeshes()
{
	return m_subMeshes;
}
