#include <StdAfx.h>
#include <Graphics/RenderScene.h>

#include <Game/World/GameObject/LiveIDSet.h>

#include <Rendering/Renderer.h>

#include <Utils/Math/Math.h>

#include <algorithm>

namespace
{
	// Never reset, so IDs are never reused (see RenderSceneLifetime)
	U32 s_nextRenderObjectID = 0;
	U32 s_nextLightObjectID = 0;

	LiveIDSet s_aliveRenderObjects;
	LiveIDSet s_aliveLightObjects;
}

namespace RenderSceneLifetime
{
	bool IsRenderObjectAlive(U32 id)
	{
		return s_aliveRenderObjects.Contains(id);
	}

	bool IsLightObjectAlive(U32 id)
	{
		return s_aliveLightObjects.Contains(id);
	}

	void CheckAccess(bool isAlive)
	{
		AssertMsg(isAlive, "Render scene handle dereferenced after its object was destroyed");
	}
}

LightObject::LightObject()
	: m_liveID(s_nextLightObjectID++)
{
	s_aliveLightObjects.Add(m_liveID);
}

LightObject::~LightObject()
{
	s_aliveLightObjects.Remove(m_liveID);
}

RenderObject::RenderObject()
	: m_liveID(s_nextRenderObjectID++)
{
	s_aliveRenderObjects.Add(m_liveID);
}

RenderObject::~RenderObject()
{
	s_aliveRenderObjects.Remove(m_liveID);
}

void LightObject::Initialize()
{
	m_propertyBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(PropertyBufferLayout), 16, 1);
}

void RenderObject::SetStaticMesh(const StaticMeshInstance& staticMesh)
{
	m_staticMesh = staticMesh;
	m_localBounds = m_staticMesh.GetSubMeshes().empty() ? BoundingBox() : m_staticMesh.GetBounds();
	MarkChanged();
}

void RenderObject::SetTransform(const Matrix4x4& transform)
{
	if (transform != m_matrixMem.transform)
	{
		m_matrixMem.transform = transform;
		m_matrixMem.invTransform = transform.Inverse();
		MarkChanged();
	}
}

const BoundingBox& RenderObject::GetBounds() const
{
	if (m_isBoundsStale)
	{
		m_bounds = m_localBounds.Transformed(m_matrixMem.transform);
		m_isBoundsStale = false;
	}
	return m_bounds;
}

const std::vector<RenderObject::SubMeshBounds>& RenderObject::GetSubMeshBounds() const
{
	if (m_areSubMeshBoundsStale)
	{
		const std::vector<MeshGeometry>& subMeshes = m_staticMesh.GetSubMeshes();
		m_subMeshBounds.resize(subMeshes.size());
		for (size_t subMeshIndex = 0; subMeshIndex < subMeshes.size(); ++subMeshIndex)
		{
			SubMeshBounds& bounds = m_subMeshBounds[subMeshIndex];
			bounds.Corners = subMeshes[subMeshIndex].GetBounds().GetTransformedCorners(m_matrixMem.transform);
			bounds.Bounds = BoundingBox::Around(bounds.Corners);
		}
		m_areSubMeshBoundsStale = false;
	}
	return m_subMeshBounds;
}

void RenderObject::MarkChanged()
{
	m_isBoundsStale = true;
	m_areSubMeshBoundsStale = true;
	if (m_scene != nullptr)
	{
		m_scene->OnRenderObjectChanged();
	}
}

void RenderScene::ClearObjects()
{
	m_renderObjects.clear();
	OnRenderObjectChanged();
}

const BoundingBox& RenderScene::GetBounds() const
{
	if (m_isBoundsStale)
	{
		m_bounds = BoundingBox();
		for (const std::unique_ptr<RenderObject>& renderObject : m_renderObjects)
		{
			m_bounds.Grow(renderObject->GetBounds());
		}
		m_isBoundsStale = false;
	}
	return m_bounds;
}

void RenderScene::OnRenderObjectChanged()
{
	++m_revision;
	m_isBoundsStale = true;
}

RenderObjectPtr RenderScene::CreateRenderObject(const StaticMeshInstance& staticMesh)
{
	OPTICK_EVENT();
	
	std::unique_ptr<RenderObject>& renderObjectPtr = m_renderObjects.emplace_back(std::make_unique<RenderObject>());
	renderObjectPtr->m_scene = this;
	// Marks the scene changed
	renderObjectPtr->SetStaticMesh(staticMesh);

	return RenderObjectPtr(renderObjectPtr.get(), renderObjectPtr->GetLiveID());
}

void RenderScene::DestroyRenderObject(RenderObjectPtr& renderObject)
{
	OPTICK_EVENT();

	// Already gone (e.g. ClearObjects): its address may belong to a new object by now, so it isn't searched for
	if (!renderObject.IsValid())
	{
		renderObject = RenderObjectPtr();
		return;
	}

	RenderObjectContainer& renderObjects = m_renderObjects;
	auto iter = std::find_if(renderObjects.begin(), renderObjects.end(),
		[&renderObject](const std::unique_ptr<RenderObject>& other)
		{
			return renderObject.m_ptr == other.get();
		});

	if (iter != renderObjects.end())
	{
		if (renderObjects.size() > 1)
		{
			std::iter_swap(iter, std::prev(renderObjects.end()));
			renderObjects.erase(std::prev(renderObjects.end()));
		}
		else
		{
			renderObjects.erase(iter);
		}

		renderObject = RenderObjectPtr();
		OnRenderObjectChanged();
	}
}

LightObjectPtr RenderScene::CreateLightObject()
{
	std::unique_ptr<LightObject>& lightObjectPtr = m_lightObjects.emplace_back(std::make_unique<LightObject>());
	lightObjectPtr->m_sceneRevision = &m_revision;
	lightObjectPtr->Initialize();
	++m_revision;

	return LightObjectPtr(lightObjectPtr.get(), lightObjectPtr->GetLiveID());
}

void RenderScene::DestroyLightObject(LightObjectPtr& lightObject)
{
	OPTICK_EVENT();

	// Already gone: its address may belong to a new light by now, so it isn't searched for
	if (!lightObject.IsValid())
	{
		lightObject = LightObjectPtr();
		return;
	}

	LightObjectContainer& lightObjects = m_lightObjects;
	auto iter = std::find_if(lightObjects.begin(), lightObjects.end(),
		[&lightObject](const std::unique_ptr<LightObject>& other)
		{
			return lightObject.m_ptr == other.get();
		});

	if (iter != lightObjects.end())
	{
		if (lightObjects.size() > 1)
		{
			std::iter_swap(iter, std::prev(lightObjects.end()));
			lightObjects.erase(std::prev(lightObjects.end()));
		}
		else
		{
			lightObjects.erase(iter);
		}

		lightObject = LightObjectPtr();
		++m_revision;
	}
}

void RenderScene::DrawLine(const Vector3D& start, const Vector3D& end, const Vector3D& colour, float alpha, const DebugLineFade& fade)
{
	DebugLine line{ start, end, colour, alpha, fade };
	m_debugLines.emplace_back(std::move(line));
}

void RenderScene::DrawWireBox(const BoundingBox& box, const Matrix4x4& transform, const Vector3D& colour, float alpha)
{
	// Transformed one by one, so the box follows the object's rotation and scale
	const BoundingBox::Corners corners = box.GetTransformedCorners(transform);

	// Edges connect corners that differ by exactly one axis bit (BoundingBox's corner order)
	for (int i = 0; i < 8; ++i)
	{
		for (int axisBit = 1; axisBit < 8; axisBit <<= 1)
		{
			if ((i & axisBit) == 0)
			{
				DrawLine(corners[i], corners[i | axisBit], colour, alpha);
			}
		}
	}
}
