#include <StdAfx.h>
#include <Graphics/StaticMeshInstance.h>

#include <Utils/DebugUtils/Debug.h>

std::shared_ptr<StaticMeshInstance> StaticMeshInstance::Create(std::shared_ptr<const StaticMesh> mesh, ResourceHandle meshResource)
{
	AssertNotNull(mesh);
	// The constructor is private, so make_shared can't call it
	return std::shared_ptr<StaticMeshInstance>(new StaticMeshInstance(std::move(mesh), std::move(meshResource)));
}

StaticMeshInstance::StaticMeshInstance(std::shared_ptr<const StaticMesh> mesh, ResourceHandle meshResource)
	: m_mesh(std::move(mesh))
	, m_meshResource(std::move(meshResource))
{
}

const Filepath& StaticMeshInstance::GetResourcePath() const
{
	static const Filepath s_noPath;
	return m_meshResource.IsValid() ? m_meshResource.GetResourcePath() : s_noPath;
}
