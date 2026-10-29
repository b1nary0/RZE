#include <StdAfx.h>
#include <Graphics/RenderScene.h>

#include <Utils/LiveIDAllocator.h>
#include <Utils/Math/Math.h>
#include <Utils/Math/Units.h>

#include <algorithm>
#include <cmath>

namespace
{
	// Hands out every render object and light live ID (see RenderSceneLifetime). Made on first use and never destroyed,
	// so it works for objects made during static initialization or destroyed after main returns.
	LiveIDAllocator& AliveObjects()
	{
		static LiveIDAllocator* const s_allocator = new LiveIDAllocator();
		return *s_allocator;
	}
}

bool RenderSceneLifetime::IsAlive(U64 id)
{
	return AliveObjects().Contains(id);
}

U64 RenderSceneLifetime::Acquire()
{
	return AliveObjects().Acquire();
}

void RenderSceneLifetime::Release(U64 id)
{
	AliveObjects().Release(id);
}

U64 RenderSceneLifetime::GetID(const RenderObject* renderObject)
{
	return renderObject->GetLiveID();
}

U64 RenderSceneLifetime::GetID(const LightObject* lightObject)
{
	return lightObject->GetLiveID();
}

U64 RenderSceneLifetime::GetID(const ReflectionProbeObject* reflectionProbe)
{
	return reflectionProbe->GetLiveID();
}

void RenderScene::ClearObjects()
{
	m_renderObjects.clear();
	OnAllRenderObjectsChanged();
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

const BoundingBox& RenderScene::GetStaticBounds() const
{
	if (m_isStaticBoundsStale)
	{
		m_staticBounds = BoundingBox();
		for (const std::unique_ptr<RenderObject>& renderObject : m_renderObjects)
		{
			if (renderObject->GetMobility() == ERenderMobility::Static)
			{
				m_staticBounds.Grow(renderObject->GetBounds());
			}
		}
		m_isStaticBoundsStale = false;
	}
	return m_staticBounds;
}

void RenderScene::OnRenderObjectChanged(ERenderMobility mobility, const BoundingBox& oldBounds, const BoundingBox& newBounds)
{
	m_objectsRevision = ++m_revision;
	m_isBoundsStale = true;

	// Logged apart, so an object that jumps across the scene doesn't mark everything between
	if (mobility == ERenderMobility::Static)
	{
		m_staticObjectsRevision = m_revision;
		m_isStaticBoundsStale = true;
		m_staticChanges.Add(m_revision, oldBounds);
		m_staticChanges.Add(m_revision, newBounds);
	}
	else
	{
		m_movableChanges.Add(m_revision, oldBounds);
		m_movableChanges.Add(m_revision, newBounds);
	}
}

void RenderScene::OnRenderObjectMobilityChanged(const BoundingBox& bounds)
{
	m_objectsRevision = ++m_revision;
	m_staticObjectsRevision = m_revision;
	m_isStaticBoundsStale = true;
	m_staticChanges.Add(m_revision, bounds);
	m_movableChanges.Add(m_revision, bounds);
}

void RenderScene::OnAllRenderObjectsChanged()
{
	m_objectsRevision = ++m_revision;
	m_staticObjectsRevision = m_revision;
	m_isBoundsStale = true;
	m_isStaticBoundsStale = true;
	m_staticChanges.AddEverywhere(m_revision);
	m_movableChanges.AddEverywhere(m_revision);
}

RenderObjectPtr RenderScene::CreateRenderObject(const std::shared_ptr<const StaticMeshInstance>& meshInstance)
{
	OPTICK_EVENT();
	
	std::unique_ptr<RenderObject>& renderObjectPtr = m_renderObjects.emplace_back(std::make_unique<RenderObject>());
	renderObjectPtr->m_scene = this;
	// Marks the scene changed
	renderObjectPtr->SetMeshInstance(meshInstance);

	return RenderObjectPtr(renderObjectPtr.get());
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
			return renderObject == other.get();
		});

	if (iter != renderObjects.end())
	{
		const BoundingBox oldBounds = (*iter)->GetBounds();
		const ERenderMobility mobility = (*iter)->GetMobility();

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
		OnRenderObjectChanged(mobility, oldBounds, BoundingBox());
	}
}

ReflectionProbeObjectPtr RenderScene::CreateReflectionProbe()
{
	std::unique_ptr<ReflectionProbeObject>& reflectionProbe = m_reflectionProbes.emplace_back(std::make_unique<ReflectionProbeObject>(m_revision));
	return ReflectionProbeObjectPtr(reflectionProbe.get());
}

void RenderScene::DestroyReflectionProbe(ReflectionProbeObjectPtr& reflectionProbe)
{
	// Already gone: its address may belong to a new probe by now, so it isn't searched for
	if (!reflectionProbe.IsValid())
	{
		reflectionProbe = ReflectionProbeObjectPtr();
		return;
	}

	// In the order they were made, which is the order ReflectionProbeStage gives them its slots
	auto iter = std::find_if(m_reflectionProbes.begin(), m_reflectionProbes.end(),
		[&reflectionProbe](const std::unique_ptr<ReflectionProbeObject>& other)
		{
			return reflectionProbe == other.get();
		});
	if (iter != m_reflectionProbes.end())
	{
		m_reflectionProbes.erase(iter);
		reflectionProbe = ReflectionProbeObjectPtr();
		++m_revision;
	}
}

LightObjectPtr RenderScene::CreateLightObject(ELightType type)
{
	std::unique_ptr<LightObject>& lightObjectPtr = m_lightObjects.emplace_back(std::make_unique<LightObject>(type));
	lightObjectPtr->m_sceneRevision = &m_revision;
	lightObjectPtr->m_lightChanges = &m_lightChanges;
	// Lights where it reaches from now on
	lightObjectPtr->MarkChanged(true);

	return LightObjectPtr(lightObjectPtr.get());
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
			return lightObject == other.get();
		});

	if (iter != lightObjects.end())
	{
		// No longer lights where it reached
		(*iter)->MarkChanged(true);

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
	}
}

const LightObject* RenderScene::GetDirectionalLight() const
{
	for (const std::unique_ptr<LightObject>& lightObject : m_lightObjects)
	{
		if (lightObject->GetType() == ELightType::Directional)
		{
			return lightObject.get();
		}
	}
	return nullptr;
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

void RenderScene::DrawWireSphere(const Vector3D& centre, float radius, const Vector3D& colour, float alpha, float occludedAlpha)
{
	// Enough that a sphere filling the view still reads as round
	constexpr int k_segmentCount = 48;

	// Each circle is drawn in the plane of two axes
	const Vector3D axes[3] = { WorldAxes::Right(), WorldAxes::Up(), WorldAxes::Forward() };
	for (int circle = 0; circle < 3; ++circle)
	{
		const Vector3D& u = axes[circle];
		const Vector3D& v = axes[(circle + 1) % 3];

		Vector3D previous = centre + u * radius;
		for (int segment = 1; segment <= k_segmentCount; ++segment)
		{
			const float angle = segment * (2.0f * MathUtils::Pi / k_segmentCount);
			const Vector3D next = centre + (u * std::cos(angle) + v * std::sin(angle)) * radius;
			m_debugLines.push_back({ previous, next, colour, alpha, DebugLineFade(), occludedAlpha });
			previous = next;
		}
	}
}
