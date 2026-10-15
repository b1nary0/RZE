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
	std::vector<MeshPlacement> placements(meshGeometry.size());
	for (size_t subMeshIndex = 0; subMeshIndex < placements.size(); ++subMeshIndex)
	{
		placements[subMeshIndex].GeometryIndex = static_cast<U32>(subMeshIndex);
	}

	Initialize(meshGeometry, std::move(placements));
}

void StaticMeshInstance::Initialize(const std::vector<MeshGeometry>& meshGeometry, std::vector<MeshPlacement>&& placements)
{
	m_subMeshes = meshGeometry;
	m_placements = std::move(placements);

	m_isAtOrigin.resize(m_placements.size());
	for (size_t placementIndex = 0; placementIndex < m_placements.size(); ++placementIndex)
	{
		const MeshPlacement& placement = m_placements[placementIndex];
		AssertExpr(placement.GeometryIndex < m_subMeshes.size());
		m_isAtOrigin[placementIndex] = placement.LocalTransform == Matrix4x4::IDENTITY;
	}

	CalculateBounds();
}

void StaticMeshInstance::BuildDraws(const MatrixMem& objectMatrices, std::vector<MeshDraw>& outDraws) const
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

void StaticMeshInstance::CalculateBounds()
{
	if (m_placements.empty())
	{
		m_bounds = BoundingBox(Vector3D(), Vector3D());
		return;
	}

	m_bounds = BoundingBox();
	for (const MeshPlacement& placement : m_placements)
	{
		m_bounds.Grow(m_subMeshes[placement.GeometryIndex].GetBounds().Transformed(placement.LocalTransform));
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
