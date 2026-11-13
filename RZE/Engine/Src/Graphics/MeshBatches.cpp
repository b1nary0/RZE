#include <Graphics/MeshBatches.h>

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
}

void MeshBatches::Add(const MeshDraw& draw, const void* bindKey)
{
	m_grouping.Add(draw, bindKey);
}

void MeshBatches::AddUngrouped(const MeshDraw& draw)
{
	m_grouping.AddUngrouped(draw);
}

void MeshBatches::Upload()
{
	m_grouping.Build();
	m_instances.Build(m_grouping.GetPlacements());
	m_instances.Upload();
}

void MeshBatches::BindInstances() const
{
	m_instances.Bind();
}
