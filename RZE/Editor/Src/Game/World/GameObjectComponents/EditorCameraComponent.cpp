#include <StdAfx.h>
#include <Game/World/GameObjectComponents/EditorCameraComponent.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Graphics/RenderEngine.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Math.h>

namespace
{
	constexpr float k_cameraMaxZoomSpeed = 0.5f;
	constexpr float k_cameraMaxSpeed = 8.0f;
}

const Vector3D& EditorCameraComponent::GetLookAt() const
{
	return m_lookAt;
}

const Vector3D& EditorCameraComponent::GetUpDir() const
{
	return m_upDir;
}

const Vector3D& EditorCameraComponent::GetForward() const
{
	return m_forward;
}

const Matrix4x4& EditorCameraComponent::GetProjectionMatrix() const
{
	return m_projectionMat;
}

const Matrix4x4& EditorCameraComponent::GetViewMatrix() const
{
	return m_viewMat;
}

float EditorCameraComponent::GetFOV() const
{
	return m_fov;
}

float EditorCameraComponent::GetAspectRatio() const
{
	return m_aspectRatio;
}

float EditorCameraComponent::GetNearCull() const
{
	return m_nearCull;
}

float EditorCameraComponent::GetFarCull() const
{
	return m_farCull;
}

bool EditorCameraComponent::IsActiveCamera() const
{
	return m_isActiveCamera;
}

void EditorCameraComponent::SetLookAt(const Vector3D& lookAt)
{
	m_lookAt = lookAt;
}

void EditorCameraComponent::SetUpDir(const Vector3D& upDir)
{
	m_upDir = upDir;
}

void EditorCameraComponent::SetForward(const Vector3D& forward)
{
	m_forward = forward;
}

void EditorCameraComponent::SetFOV(float fov)
{
	m_fov = fov;
}

void EditorCameraComponent::SetAspectRatio(float aspectRatio)
{
	m_aspectRatio = aspectRatio;
}

void EditorCameraComponent::SetNearCull(float nearCull)
{
	m_nearCull = nearCull;
}

void EditorCameraComponent::SetFarCull(float farCull)
{
	m_farCull = farCull;
}

void EditorCameraComponent::SetAsActiveCamera(bool isActiveCamera)
{
	m_isActiveCamera = isActiveCamera;
}

void EditorCameraComponent::Initialize()
{
	GetOwner()->SetIncludeInSave(false);
}

void EditorCameraComponent::OnAddToScene()
{
	// #TODO This wont work long term, just trying to get things rendering with new code
	m_isActiveCamera = true;
	m_aspectRatio = RZE().GetWindowSize().X() / RZE().GetWindowSize().Y();
}

void EditorCameraComponent::Update()
{
	if (IsActiveCamera())
	{
		GameObjectComponentPtr<TransformComponent> transformComponent = GetOwner()->GetComponent<TransformComponent>();
		AssertMsg(transformComponent != nullptr, "A camera without a transform is useless");

		KeyboardInput(transformComponent);
		MouseInput(transformComponent);

		GenerateCameraMatrices(transformComponent->GetPosition());
		{
			// Push data to RenderEngine
			RenderCamera& renderCamera = RZE().GetRenderEngine().GetCamera();
			renderCamera.Position = transformComponent->GetPosition();
			renderCamera.ClipSpace = GetProjectionMatrix() * GetViewMatrix();
		}
	}
}

void EditorCameraComponent::GenerateCameraMatrices(const Vector3D& position)
{
	OPTICK_EVENT("GenerateCameraMatrices");

	m_projectionMat = Matrix4x4::CreatePerspectiveMatrix(m_fov, m_aspectRatio, m_nearCull, m_farCull);
	m_viewMat = Matrix4x4::CreateViewMatrix(position, position + m_forward, m_upDir);
}

void EditorCameraComponent::KeyboardInput(GameObjectComponentPtr<TransformComponent>& transfComp)
{
	InputHandler& inputHandler = RZE().GetInputHandler();

	float dt = static_cast<float>(RZE().GetDeltaTime());

	float speedDelta = (m_speed * m_deltaSpeedRampMultiplier) * dt;

	// TODO(Josh::The extra condition here for hold is because of the frame delay for ::Hold. Should fix eventually.)
	if (inputHandler.GetMouseState().GetButtonState(EMouseButton::MouseButton_Right) == EButtonState::ButtonState_Pressed)
	{
		bool hasValidMovementInput = false;

		if (inputHandler.GetKeyboardState().GetButtonState(Win32KeyCode::Key_W) == EButtonState::ButtonState_Pressed
			|| inputHandler.GetKeyboardState().GetButtonState(Win32KeyCode::Key_W) == EButtonState::ButtonState_Hold)
		{
			transfComp->GetPosition() += m_forward * speedDelta;
			hasValidMovementInput = true;
		}
		else if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_S])
		{
			transfComp->GetPosition() -= m_forward * speedDelta;
			hasValidMovementInput = true;
		}

		if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_A])
		{
			transfComp->GetPosition() -= m_forward.Cross(m_upDir).Normalize() * speedDelta;
			hasValidMovementInput = true;
		}
		else if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_D])
		{
			transfComp->GetPosition() += m_forward.Cross(m_upDir).Normalize() * speedDelta;
			hasValidMovementInput = true;
		}

		if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_E])
		{
			transfComp->GetPosition() += m_forward.Cross(m_upDir).Cross(m_forward) * speedDelta;
			hasValidMovementInput = true;
		}
		else if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_Q])
		{
			transfComp->GetPosition() -= m_forward.Cross(m_upDir).Cross(m_forward) * speedDelta;
			hasValidMovementInput = true;
		}

		if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_1])
		{
			// Focus object
			hasValidMovementInput = true;
		}

		SetSpeedRampMultiplier(hasValidMovementInput, m_deltaSpeedRampMultiplier + dt);
	}

	// #TODO(Josh::Support for special keys CTRL SHIFT etc)
	if (inputHandler.GetKeyboardState().GetButtonState(Win32KeyCode::Key_R) == EButtonState::ButtonState_Pressed)
	{
		m_speed = k_cameraMaxSpeed * 4.0f;
	}
	else if (inputHandler.GetKeyboardState().GetButtonState(Win32KeyCode::Space) == EButtonState::ButtonState_Pressed)
	{
		m_speed = k_cameraMaxSpeed / 8.0f;
	}

	if (!inputHandler.GetKeyboardState().IsDownThisFrame(Win32KeyCode::Space) && !inputHandler.GetKeyboardState().IsDownThisFrame(Win32KeyCode::Key_R))
	{
		m_speed = k_cameraMaxSpeed;
	}

	Int32 wheelVal = RZE().GetInputHandler().GetMouseState().CurWheelVal;
	if (wheelVal != 0)
	{
		wheelVal = MathUtils::Clamp(wheelVal, -1, 1);
		transfComp->GetPosition() = transfComp->GetPosition() + (m_forward * static_cast<float>(wheelVal)) * k_cameraMaxZoomSpeed;
	}
}

void EditorCameraComponent::MouseInput(GameObjectComponentPtr<TransformComponent>& transfComp)
{
	InputHandler& inputHandler = RZE().GetInputHandler();

	const Vector3D& curPos = inputHandler.GetMouseState().CurPosition;

	Int32 wheelVal = inputHandler.GetMouseState().CurWheelVal;
	if (wheelVal != 0)
	{
		wheelVal = MathUtils::Clamp(wheelVal, -1, 1);
		transfComp->GetPosition() = transfComp->GetPosition() + (m_forward * static_cast<float>(wheelVal)) * k_cameraMaxZoomSpeed;
	}

	if (inputHandler.GetMouseState().GetButtonState(EMouseButton::MouseButton_Right) == EButtonState::ButtonState_Pressed)
	{
		Vector2D diff = curPos - m_mousePrevPos;

		const bool withSensitivity = true;
		CalculateNewForward(diff, withSensitivity);
	}

	m_mousePrevPos = curPos;
}

void EditorCameraComponent::SetSpeedRampMultiplier(bool isMoving, float growingDelta)
{
	if (isMoving)
	{
		m_deltaSpeedRampMultiplier = MathUtils::Clampf(growingDelta, kMinDirectionHeldTime, kMaxDirectionHeldTime);
	}
	else
	{
		m_deltaSpeedRampMultiplier = kMinDirectionHeldTime;
	}
}

void EditorCameraComponent::CalculateNewForward(const Vector2D& delta, bool withSensitivity)
{
	Vector2D outDelta = delta;

	if (withSensitivity)
	{
		const float sensitivity = 0.1f;
		outDelta *= sensitivity;
	}

	m_yawPitch += outDelta;

	float yawInRadians = m_yawPitch.X() * MathUtils::ToRadians;
	float pitchInRadians = m_yawPitch.Y() * MathUtils::ToRadians;

	Vector3D newForward;
	newForward.SetX(std::cos(yawInRadians) * std::cos(pitchInRadians));
	newForward.SetY(-std::sin(pitchInRadians));
	newForward.SetZ(std::sin(yawInRadians) * std::cos(pitchInRadians));

	m_forward = newForward;
	m_forward.Normalize();
}
