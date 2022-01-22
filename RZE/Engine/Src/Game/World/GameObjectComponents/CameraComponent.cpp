#include <StdAfx.h>
#include <Game/World/GameObjectComponents/CameraComponent.h>

#include <Utils/DebugUtils/Debug.h>

const Vector3D& CameraComponent::GetLookAt() const
{
	return m_lookAt;
}

const Vector3D& CameraComponent::GetUpDir() const
{
	return m_upDir;
}

const Vector3D& CameraComponent::GetForward() const
{
	return m_forward;
}

const Matrix4x4& CameraComponent::GetProjectionMatrix() const
{
	return m_projectionMat;
}

const Matrix4x4& CameraComponent::GetViewMatrix() const
{
	return m_viewMat;
}

float CameraComponent::GetFOV() const
{
	return m_fov;
}

float CameraComponent::GetAspectRatio() const
{
	return m_aspectRatio;
}

float CameraComponent::GetNearCull() const
{
	return m_nearCull;
}

float CameraComponent::GetFarCull() const
{
	return m_farCull;
}

bool CameraComponent::IsActiveCamera() const
{
	return m_isActiveCamera;
}

void CameraComponent::SetLookAt(const Vector3D& lookAt)
{
	m_lookAt = lookAt;
}

void CameraComponent::SetUpDir(const Vector3D& upDir)
{
	m_upDir = upDir;
}

void CameraComponent::SetForward(const Vector3D& forward)
{
	m_forward = forward;
}

void CameraComponent::SetFOV(float fov)
{
	AssertExpr(fov > 0.0f);
	m_fov = fov;
}

void CameraComponent::SetAspectRatio(float aspectRatio)
{
	AssertExpr(aspectRatio > 0.0f);
	m_aspectRatio = aspectRatio;
}

void CameraComponent::SetNearCull(float nearCull)
{
	AssertExpr(nearCull > 0.0f);
	m_nearCull = nearCull;
}

void CameraComponent::SetFarCull(float farCull)
{
	m_farCull = farCull;
}

void CameraComponent::SetAsActiveCamera(bool isActiveCamera)
{
	m_isActiveCamera = isActiveCamera;
}

void CameraComponent::Save(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
{
}

void CameraComponent::Load(const rapidjson::Value& data)
{
}
