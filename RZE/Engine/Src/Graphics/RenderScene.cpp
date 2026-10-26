#include <StdAfx.h>
#include <Graphics/RenderScene.h>

#include <Utils/LiveIDAllocator.h>
#include <Utils/Math/Math.h>
#include <Utils/Math/Units.h>

#include <algorithm>
#include <cmath>

namespace
{
	// Hands out every render object and light live ID (see RenderSceneLifetime)
	LiveIDAllocator s_aliveObjects;

	// Mirrors PointFalloffSharpness in Common/Lighting.hlsli
	constexpr float k_pointFalloffSharpness = 8.0f;
	// Mirror AmbientSky and AmbientGround in Common/Lighting.hlsli; linear
	constexpr float k_ambientSky[3] = { 0.14f, 0.16f, 0.20f };
	constexpr float k_ambientGround[3] = { 0.05f, 0.045f, 0.04f };

	// Rec. 709 weights, for linear colours
	float Luminance(float r, float g, float b)
	{
		return 0.2126f * r + 0.7152f * g + 0.0722f * b;
	}

	// As SRGBToLinear in Common/ColourSpace.hlsli
	float SRGBToLinear(float value)
	{
		return std::pow(std::max(value, 0.0f), 2.2f);
	}
}

namespace RenderSceneLifetime
{
	bool IsAlive(U64 id)
	{
		return s_aliveObjects.Contains(id);
	}

	void CheckAccess(U64 id)
	{
		AssertMsg(IsAlive(id), "Render scene handle dereferenced after its object was destroyed");
	}
}

LightObject::LightObject(ELightType type)
	: m_type(type)
	, m_liveID(s_aliveObjects.Acquire())
{
}

LightObject::~LightObject()
{
	s_aliveObjects.Release(m_liveID);
}

RenderObject::RenderObject()
	: m_liveID(s_aliveObjects.Acquire())
{
}

RenderObject::~RenderObject()
{
	s_aliveObjects.Release(m_liveID);
}

void LightObject::SetType(ELightType type)
{
	if (type != m_type)
	{
		m_type = type;
		MarkShadowsChanged();
	}
}

void LightObject::SetPosition(const Vector3D& position)
{
	if (position != m_position)
	{
		m_position = position;
		if (m_type == ELightType::Point)
		{
			MarkShadowsChanged();
		}
	}
}

void LightObject::SetDirection(const Vector3D& direction)
{
	if (direction != m_direction)
	{
		m_direction = direction;
		if (m_type == ELightType::Directional)
		{
			MarkShadowsChanged();
		}
	}
}

float LightObject::CalculatePointFalloff(float rangeRatio)
{
	const float rangeRatioSq = rangeRatio * rangeRatio;
	const float window = std::clamp(1.0f - rangeRatioSq * rangeRatioSq, 0.0f, 1.0f);
	return window * window / (1.0f + k_pointFalloffSharpness * rangeRatioSq);
}

float LightObject::CalculateOutshinesAmbientRatio(float strength, const Vector4D& colour)
{
	// What a wall gets: halfway between the sky above and the ground below (HemisphereAmbient)
	const float ambient = Luminance(
		(k_ambientSky[0] + k_ambientGround[0]) * 0.5f,
		(k_ambientSky[1] + k_ambientGround[1]) * 0.5f,
		(k_ambientSky[2] + k_ambientGround[2]) * 0.5f);
	// Right next to the light, before falloff (LightRadiance)
	const float light = strength * Luminance(SRGBToLinear(colour.X()), SRGBToLinear(colour.Y()), SRGBToLinear(colour.Z()));

	if (light <= ambient)
	{
		return 0.0f;
	}

	// The falloff only goes down between the light and its range, so halving the interval homes in on the crossing
	float low = 0.0f;
	float high = 1.0f;
	for (int step = 0; step < 24; ++step)
	{
		const float middle = (low + high) * 0.5f;
		(light * CalculatePointFalloff(middle) > ambient ? low : high) = middle;
	}
	return (low + high) * 0.5f;
}

void LightObject::SetRange(float range)
{
	if (range != m_range)
	{
		m_range = range;
		if (m_type == ELightType::Point)
		{
			MarkShadowsChanged();
		}
	}
}

void LightObject::MarkShadowsChanged()
{
	if (m_sceneRevision != nullptr)
	{
		m_shadowRevision = ++*m_sceneRevision;
	}
}

void RenderObject::SetMeshInstance(const std::shared_ptr<const StaticMeshInstance>& meshInstance)
{
	const BoundingBox oldBounds = GetBounds();
	m_meshInstance = meshInstance;
	const bool hasSubMeshes = m_meshInstance != nullptr && !m_meshInstance->GetMesh().GetSubMeshes().empty();
	m_localBounds = hasSubMeshes ? m_meshInstance->GetMesh().GetBounds() : BoundingBox();
	// The draws still point into the old mesh until GetDraws rebuilds them, which is the only way to read them
	MarkChanged(oldBounds);
}

void RenderObject::SetTransform(const Matrix4x4& transform)
{
	if (transform != m_matrixMem.transform)
	{
		const BoundingBox oldBounds = GetBounds();
		m_matrixMem.transform = transform;
		m_matrixMem.invTransform = transform.Inverse();
		MarkChanged(oldBounds);
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

const std::vector<MeshDraw>& RenderObject::GetDraws() const
{
	if (m_areDrawsStale)
	{
		if (m_meshInstance != nullptr)
		{
			m_meshInstance->BuildDraws(m_matrixMem, m_draws);
		}
		else
		{
			m_draws.clear();
		}
		m_areDrawsStale = false;
	}
	return m_draws;
}

void RenderObject::MarkChanged(const BoundingBox& oldBounds)
{
	m_isBoundsStale = true;
	m_areDrawsStale = true;
	if (m_scene != nullptr)
	{
		m_scene->OnRenderObjectChanged(oldBounds, GetBounds());
	}
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

void RenderScene::OnRenderObjectChanged(const BoundingBox& oldBounds, const BoundingBox& newBounds)
{
	m_objectsRevision = ++m_revision;
	m_isBoundsStale = true;

	// Logged apart, so an object that jumps across the scene doesn't mark everything between
	m_changeLog.Add(m_revision, oldBounds);
	m_changeLog.Add(m_revision, newBounds);
}

void RenderScene::OnAllRenderObjectsChanged()
{
	m_objectsRevision = ++m_revision;
	m_isBoundsStale = true;
	m_changeLog.AddEverywhere(m_revision);
}

RenderObjectPtr RenderScene::CreateRenderObject(const std::shared_ptr<const StaticMeshInstance>& meshInstance)
{
	OPTICK_EVENT();
	
	std::unique_ptr<RenderObject>& renderObjectPtr = m_renderObjects.emplace_back(std::make_unique<RenderObject>());
	renderObjectPtr->m_scene = this;
	// Marks the scene changed
	renderObjectPtr->SetMeshInstance(meshInstance);

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
		const BoundingBox oldBounds = (*iter)->GetBounds();

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
		OnRenderObjectChanged(oldBounds, BoundingBox());
	}
}

LightObjectPtr RenderScene::CreateLightObject(ELightType type)
{
	std::unique_ptr<LightObject>& lightObjectPtr = m_lightObjects.emplace_back(std::make_unique<LightObject>(type));
	lightObjectPtr->m_sceneRevision = &m_revision;
	lightObjectPtr->m_shadowRevision = ++m_revision;

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
