#include <StdAfx.h>
#include <Graphics/StaticMesh.h>

#include <Utils/DebugUtils/Debug.h>

std::shared_ptr<const StaticMesh> StaticMesh::Create(std::string name, std::vector<MeshGeometry> subMeshes)
{
	std::vector<MeshPlacement> placements(subMeshes.size());
	for (size_t subMeshIndex = 0; subMeshIndex < placements.size(); ++subMeshIndex)
	{
		placements[subMeshIndex].GeometryIndex = static_cast<U32>(subMeshIndex);
	}

	return Create(std::move(name), std::move(subMeshes), std::move(placements));
}

std::shared_ptr<const StaticMesh> StaticMesh::Create(std::string name, std::vector<MeshGeometry> subMeshes, std::vector<MeshPlacement> placements)
{
	// The constructor is private, so make_shared can't call it
	return std::shared_ptr<const StaticMesh>(new StaticMesh(std::move(name), std::move(subMeshes), std::move(placements)));
}

StaticMesh::StaticMesh(std::string name, std::vector<MeshGeometry> subMeshes, std::vector<MeshPlacement> placements)
	: m_name(std::move(name))
	, m_subMeshes(std::move(subMeshes))
	, m_placements(std::move(placements))
{
	m_isAtOrigin.resize(m_placements.size());
	for (size_t placementIndex = 0; placementIndex < m_placements.size(); ++placementIndex)
	{
		const MeshPlacement& placement = m_placements[placementIndex];
		AssertExpr(placement.GeometryIndex < m_subMeshes.size());
		m_isAtOrigin[placementIndex] = placement.LocalTransform == Matrix4x4::IDENTITY;
	}

	if (m_placements.empty())
	{
		m_bounds = BoundingBox(Vector3D(), Vector3D());
		return;
	}

	for (const MeshPlacement& placement : m_placements)
	{
		m_bounds.Grow(m_subMeshes[placement.GeometryIndex].GetBounds().Transformed(placement.LocalTransform));
	}
}

void StaticMesh::BuildDraws(const MatrixMem& objectMatrices, std::vector<MeshDraw>& outDraws) const
{
	outDraws.resize(m_placements.size());
	for (size_t placementIndex = 0; placementIndex < m_placements.size(); ++placementIndex)
	{
		const MeshPlacement& placement = m_placements[placementIndex];
		MeshDraw& draw = outDraws[placementIndex];

		draw.Geometry = &m_subMeshes[placement.GeometryIndex];
		if (m_isAtOrigin[placementIndex])
		{
			draw.Transform = &objectMatrices.transform;
		}
		else
		{
			draw.OwnMatrices.transform = objectMatrices.transform * placement.LocalTransform;
			draw.OwnMatrices.invTransform = draw.OwnMatrices.transform.Inverse();
			draw.Transform = &draw.OwnMatrices.transform;
		}

		draw.Corners = draw.Geometry->GetBounds().GetTransformedCorners(*draw.Transform);
		draw.Bounds = BoundingBox::Around(draw.Corners);
	}
}
