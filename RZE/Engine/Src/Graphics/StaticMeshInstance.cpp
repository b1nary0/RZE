#include <StdAfx.h>
#include <Graphics/StaticMeshInstance.h>

#include <Utils/DebugUtils/Debug.h>

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
