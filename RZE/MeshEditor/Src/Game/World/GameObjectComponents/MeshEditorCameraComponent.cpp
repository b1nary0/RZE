#include <StdAfx.h>
#include <Game/World/GameObjectComponents/MeshEditorCameraComponent.h>

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

namespace MeshEditor
{
	const Vector3D& MeshEditorCameraComponent::GetLookAt() const
	{
		return m_lookAt;
	}

	const Vector3D& MeshEditorCameraComponent::GetUpDir() const
	{
		return m_upDir;
	}

	const Vector3D& MeshEditorCameraComponent::GetForward() const
	{
		return m_forward;
	}

	const Matrix4x4& MeshEditorCameraComponent::GetProjectionMatrix() const
	{
		return m_projectionMat;
	}

	const Matrix4x4& MeshEditorCameraComponent::GetViewMatrix() const
	{
		return m_viewMat;
	}

	float MeshEditorCameraComponent::GetFOV() const
	{
		return m_fov;
	}

	float MeshEditorCameraComponent::GetAspectRatio() const
	{
		return m_aspectRatio;
	}

	float MeshEditorCameraComponent::GetNearCull() const
	{
		return m_nearCull;
	}

	float MeshEditorCameraComponent::GetFarCull() const
	{
		return m_farCull;
	}

	bool MeshEditorCameraComponent::IsActiveCamera() const
	{
		return m_isActiveCamera;
	}

	void MeshEditorCameraComponent::SetLookAt(const Vector3D& lookAt)
	{
		m_lookAt = lookAt;
	}

	void MeshEditorCameraComponent::SetUpDir(const Vector3D& upDir)
	{
		m_upDir = upDir;
	}

	void MeshEditorCameraComponent::SetForward(const Vector3D& forward)
	{
		m_forward = forward;
	}

	void MeshEditorCameraComponent::SetFOV(float fov)
	{
		m_fov = fov;
	}

	void MeshEditorCameraComponent::SetAspectRatio(float aspectRatio)
	{
		m_aspectRatio = aspectRatio;
	}

	void MeshEditorCameraComponent::SetNearCull(float nearCull)
	{
		m_nearCull = nearCull;
	}

	void MeshEditorCameraComponent::SetFarCull(float farCull)
	{
		m_farCull = farCull;
	}

	void MeshEditorCameraComponent::SetAsActiveCamera(bool isActiveCamera)
	{
		m_isActiveCamera = isActiveCamera;
	}

	void MeshEditorCameraComponent::Initialize()
	{
		GetOwner()->SetIncludeInSave(false);
	}

	void MeshEditorCameraComponent::OnAddToScene()
	{
		// #TODO This wont work long term, just trying to get things rendering with new code
		m_isActiveCamera = true;
		m_aspectRatio = RZE().GetWindowSize().X() / RZE().GetWindowSize().Y();
	}

	void MeshEditorCameraComponent::Update()
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

	void MeshEditorCameraComponent::GenerateCameraMatrices(const Vector3D& position)
	{
		OPTICK_EVENT("GenerateCameraMatrices");

		m_projectionMat = Matrix4x4::CreatePerspectiveMatrix(m_fov, m_aspectRatio, m_nearCull, m_farCull);
		m_viewMat = Matrix4x4::CreateViewMatrix(position, position + m_forward, m_upDir);
	}

	void MeshEditorCameraComponent::KeyboardInput(GameObjectComponentPtr<TransformComponent>& transfComp)
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

	void MeshEditorCameraComponent::MouseInput(GameObjectComponentPtr<TransformComponent>& transfComp)
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
			Vector3D diff = curPos - m_mousePrevPos;

			const float sensitivity = 0.1f;
			diff *= sensitivity;
			m_yawPitchRoll += diff;

			float yawInRadians = m_yawPitchRoll.X() * MathUtils::ToRadians;
			float pitchInRadians = m_yawPitchRoll.Y() * MathUtils::ToRadians;

			Vector3D newForward;
			newForward.SetX(std::cos(yawInRadians) * std::cos(pitchInRadians));
			newForward.SetY(-std::sin(pitchInRadians));
			newForward.SetZ(std::sin(yawInRadians) * std::cos(pitchInRadians));

			m_forward = newForward;
			m_forward.Normalize();
		}

		m_mousePrevPos = curPos;
	}

	void MeshEditorCameraComponent::SetSpeedRampMultiplier(bool isMoving, float growingDelta)
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
}
