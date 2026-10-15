#include <StdAfx.h>
#include <Graphics/StaticMesh.h>

#include <Graphics/MaterialSlot.h>

#include <Utils/DebugUtils/Debug.h>

std::shared_ptr<const StaticMesh> StaticMesh::Create(const std::string& name, std::vector<SubMesh>&& subMeshes, std::vector<Slot>&& slots)
{
	std::vector<MeshPlacement> placements(subMeshes.size());
	for (size_t subMeshIndex = 0; subMeshIndex < placements.size(); ++subMeshIndex)
	{
		placements[subMeshIndex].GeometryIndex = static_cast<U32>(subMeshIndex);
	}

	return Create(name, std::move(subMeshes), std::move(slots), std::move(placements));
}

std::shared_ptr<const StaticMesh> StaticMesh::Create(const std::string& name, std::vector<SubMesh>&& subMeshes, std::vector<Slot>&& slots, std::vector<MeshPlacement>&& placements)
{
	// The constructor is private, so make_shared can't call it
	return std::shared_ptr<const StaticMesh>(new StaticMesh(name, std::move(subMeshes), std::move(slots), std::move(placements)));
}

StaticMesh::StaticMesh(const std::string& name, std::vector<SubMesh>&& subMeshes, std::vector<Slot>&& slots, std::vector<MeshPlacement>&& placements)
	: m_name(name)
	, m_subMeshes(std::move(subMeshes))
	, m_slots(std::move(slots))
	, m_placements(std::move(placements))
{
	for (const SubMesh& subMesh : m_subMeshes)
	{
		AssertExpr(subMesh.SlotIndex < m_slots.size());
	}

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
		m_bounds.Grow(m_subMeshes[placement.GeometryIndex].Geometry.GetBounds().Transformed(placement.LocalTransform));
	}
}

void StaticMesh::BuildDraws(const MatrixMem& objectMatrices, const std::vector<MaterialSlot>& materialSlots, std::vector<MeshDraw>& outDraws) const
{
	AssertExpr(materialSlots.size() == m_slots.size());

	outDraws.resize(m_placements.size());
	for (size_t placementIndex = 0; placementIndex < m_placements.size(); ++placementIndex)
	{
		const MeshPlacement& placement = m_placements[placementIndex];
		const SubMesh& subMesh = m_subMeshes[placement.GeometryIndex];
		MeshDraw& draw = outDraws[placementIndex];

		draw.Geometry = &subMesh.Geometry;
		draw.Slot = &materialSlots[subMesh.SlotIndex];
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
