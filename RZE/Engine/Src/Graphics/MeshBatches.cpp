#include <Graphics/MeshBatches.h>

#include <Graphics/MeshInstanceFormat.h>

#include <Utils/DebugUtils/Debug.h>

MeshBatches::MeshBatches(const MeshInstanceFormat& format)
	: m_instances(format)
{
}

void MeshBatches::Initialize()
{
	m_instances.Initialize();
}

void MeshBatches::Clear()
{
	m_grouping.Clear();
	m_ids.clear();
}

void MeshBatches::Add(const MeshDraw& draw, const void* bindKey, const void* orderKey)
{
	AssertExpr(!m_instances.GetFormat().CarriesID);
	m_grouping.Add(draw, bindKey, orderKey);
}

void MeshBatches::Add(const MeshDraw& draw, const void* bindKey, const void* orderKey, U64 id)
{
	AssertExpr(m_instances.GetFormat().CarriesID);
	m_grouping.Add(draw, bindKey, orderKey);
	m_ids.push_back(id);
}

void MeshBatches::AddInOrder(const MeshDraw& draw, const void* bindKey)
{
	AssertExpr(!m_instances.GetFormat().CarriesID);
	m_grouping.AddInOrder(draw, bindKey);
}

void MeshBatches::Upload()
{
	m_grouping.Build();
	m_instances.Build(m_grouping.GetPlacements(), m_ids);
	m_instances.Upload();
}

void MeshBatches::BindInstances() const
{
	m_instances.Bind();
}
