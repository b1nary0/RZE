#include <StdAfx.h>
#include <Graphics/StaticMeshInstance.h>

#include <Utils/DebugUtils/Debug.h>

std::shared_ptr<StaticMeshInstance> StaticMeshInstance::Create(const std::shared_ptr<const StaticMesh>& mesh, ResourceHandle&& meshResource)
{
	AssertNotNull(mesh);
	// The constructor is private, so make_shared can't call it
	return std::shared_ptr<StaticMeshInstance>(new StaticMeshInstance(mesh, std::move(meshResource)));
}

StaticMeshInstance::StaticMeshInstance(const std::shared_ptr<const StaticMesh>& mesh, ResourceHandle&& meshResource)
	: m_mesh(mesh)
	, m_meshResource(std::move(meshResource))
{
	m_materialSlots.reserve(m_mesh->GetSlots().size());
	for (const StaticMesh::Slot& slot : m_mesh->GetSlots())
	{
		m_materialSlots.emplace_back(slot.Name, slot.Material);
	}
}

void StaticMeshInstance::BuildDraws(const MatrixMem& objectMatrices, std::vector<MeshDraw>& outDraws) const
{
	m_mesh->BuildDraws(objectMatrices, m_materialSlots, outDraws);
}

const Filepath& StaticMeshInstance::GetResourcePath() const
{
	static const Filepath s_noPath;
	return m_meshResource.IsValid() ? m_meshResource.GetResourcePath() : s_noPath;
}
