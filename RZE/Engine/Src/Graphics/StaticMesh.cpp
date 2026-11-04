#include <StdAfx.h>
#include <Graphics/StaticMesh.h>

#include <Graphics/MaterialSlot.h>

#include <Utils/DebugUtils/Debug.h>

#include <algorithm>

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
	AssertExpr(std::all_of(m_subMeshes.begin(), m_subMeshes.end(),
		[this](const SubMesh& subMesh) { return subMesh.SlotIndex < m_slots.size(); }));

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

void StaticMesh::BuildDraws(const GraphicsLayouts::ObjectBufferLayout& objectMatrices, const std::vector<MaterialSlot>& materialSlots, std::vector<MeshDraw>& outDraws) const
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
			draw.Transform = &objectMatrices.Transform;
		}
		else
		{
			draw.OwnMatrices.Transform = objectMatrices.Transform * placement.LocalTransform;
			draw.OwnMatrices.InvTransform = draw.OwnMatrices.Transform.Inverse();
			draw.Transform = &draw.OwnMatrices.Transform;
		}

		draw.Corners = draw.Geometry->GetBounds().GetTransformedCorners(*draw.Transform);
		draw.Bounds = BoundingBox::Around(draw.Corners);
	}
}
