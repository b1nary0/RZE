#pragma once

#include <Graphics/StaticMeshInstance.h>

#include <Utils/Interfaces/Resource.h>

class StaticMeshResource : public IResource
{
public:
	StaticMeshResource();
	virtual ~StaticMeshResource();

public:
	bool Load(const Filepath& filePath) override;
	void Release() override;

public:
	const StaticMeshInstance& GetStaticMesh() const { return m_mesh; }

	[[nodiscard]]
	StaticMeshInstance GetInstance() const { return m_mesh; }

private:
	StaticMeshInstance m_mesh;
};