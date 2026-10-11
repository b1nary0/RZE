#include <StdAfx.h>
#include <Game/World/GameObjectComponents/EditorCameraComponent.h>

#include <Game/GameScene.h>
#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Graphics/RenderEngine.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Math.h>

namespace
{
	// Fraction of the distance to the orbit pivot covered per mouse wheel tick
	constexpr float k_wheelDollyFraction = 0.15f;
	// The camera never gets closer than this to its orbit pivot; zooming further pushes the pivot ahead instead
	constexpr float k_minOrbitDistance = 0.1f;
	// Ctrl+MMB drag: distance scales by e^(pixels * rate), so dragging feels the same near and far
	constexpr float k_dragDollyRate = 0.005f;
	// Degrees per pixel for RMB look and MMB orbit
	constexpr float k_lookSensitivity = 0.1f;
	// Stops look/orbit flipping over the poles, where the view matrix's up vector degenerates
	constexpr float k_maxPitch = 89.0f;
	// Metres per second, before the hold-to-accelerate ramp and the R (x4) / Space (/8) modifiers
	constexpr float k_cameraMaxSpeed = 5.0f;

	constexpr float k_focusDuration = 0.5f;
	// Shorter than k_focusDuration so the camera faces its target before it finishes moving, bringing the object into view early
	constexpr float k_focusRotateDuration = 0.3f;
	// Distance multiplier past a tight fit, so the framed object fills ~70% of the view and the surroundings stay visible
	constexpr float k_focusFramePadding = 1.4f;
	// Keeps tiny or mesh-less objects from pulling the camera right on top of them
	constexpr float k_focusMinRadius = 0.5f;
	// Degrees above the object the focused camera looks down from; 45 is a hard cap
	constexpr float k_focusMinElevation = 30.0f;
	constexpr float k_focusMaxElevation = 45.0f;
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

void EditorCameraComponent::SetUpDir(const Vector3D& upDir)
{
	m_upDir = upDir;
}

void EditorCameraComponent::SetForward(const Vector3D& forward)
{
	m_forward = forward.Normalized();

	// Keep yaw/pitch in sync, since mouse look rebuilds m_forward from them (inverse of CalculateNewForward).
	const float yaw = std::atan2(m_forward.Z(), m_forward.X()) * MathUtils::ToDegrees;
	const float pitch = -std::asin(MathUtils::Clampf(m_forward.Y(), -1.0f, 1.0f)) * MathUtils::ToDegrees;
	m_yawPitch.SetXY(yaw, pitch);
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

void EditorCameraComponent::FocusOn(const Vector3D& boundsMin, const Vector3D& boundsMax)
{
	GameObjectComponentPtr<TransformComponent> transformComponent = GetOwner()->GetComponent<TransformComponent>();
	AssertMsg(transformComponent != nullptr, "A camera without a transform is useless");

	const Vector3D center = (boundsMin + boundsMax) * 0.5f;
	const float radius = std::max((boundsMax - boundsMin).Length() * 0.5f, k_focusMinRadius);

	// Fit the bounding sphere to the narrower of the vertical and horizontal FOVs
	const float verticalHalfFov = m_fov * 0.5f * MathUtils::ToRadians;
	const float horizontalHalfFov = std::atan(std::tan(verticalHalfFov) * m_aspectRatio);
	const float halfFov = std::min(verticalHalfFov, horizontalHalfFov);
	const float distance = (radius / std::sin(halfFov)) * k_focusFramePadding;

	// Keep the heading we approached from, flattened into the XZ plane
	Vector3D heading = center - transformComponent->GetPosition();
	heading.SetY(0.0f);
	if (heading.LengthSq() <= VectorUtils::kEpsilonSq)
	{
		// Directly above or below the object; fall back to the current heading
		heading = m_forward;
		heading.SetY(0.0f);
		if (heading.LengthSq() <= VectorUtils::kEpsilonSq)
		{
			heading = WorldAxes::Forward();
		}
	}
	heading.Normalize();

	// Look down from the elevation we're already at relative to the object, kept within the allowed band
	const Vector3D toCamera = transformComponent->GetPosition() - center;
	const float toCameraLength = toCamera.Length();
	const float currentElevation = toCameraLength > VectorUtils::kEpsilon
		? std::asin(MathUtils::Clampf(toCamera.Y() / toCameraLength, -1.0f, 1.0f)) * MathUtils::ToDegrees
		: 0.0f;
	const float elevation = MathUtils::Clampf(currentElevation, k_focusMinElevation, k_focusMaxElevation) * MathUtils::ToRadians;
	const Vector3D targetForward = heading * std::cos(elevation) - WorldAxes::Up() * std::sin(elevation);

	m_focusStartPos = transformComponent->GetPosition();
	m_focusTargetPos = center - targetForward * distance;
	m_focusStartForward = m_forward;
	m_focusTargetForward = targetForward;
	m_focusElapsed = 0.0f;
	// Orbit around the focused object once the camera arrives
	m_orbitDistance = distance;
	m_isFocusing = true;
}

EditorCameraComponent::EditorCameraComponent()
{
	REFLECT_REGISTER_COMPONENT_CHILD(EditorCameraComponent, CameraComponent);
}

void EditorCameraComponent::Initialize()
{
	GetOwner()->SetIncludeInSave(false);

	GameObjectComponentPtr<TransformComponent> transformComponent = GetOwner()->GetComponent<TransformComponent>();
	AssertMsg(transformComponent != nullptr, "A camera without a transform is useless");

	Vector3D toSceneCenter = RZE().GetActiveScene().CalculateSceneCenter() - transformComponent->GetPosition();
	if (toSceneCenter.LengthSq() > 0.0f)
	{
		SetForward(toSceneCenter);
		m_orbitDistance = std::max(toSceneCenter.Length(), k_minOrbitDistance);
	}
	else
	{
		SetForward(m_forward);
	}
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

		UpdateFocus(transformComponent);
		KeyboardInput(transformComponent);
		MouseInput(transformComponent);

		GenerateCameraMatrices(transformComponent->GetPosition());
		{
			// Push data to RenderEngine
			RenderCamera& renderCamera = RZE().GetRenderEngine().GetMainView().Camera;
			renderCamera.Position = transformComponent->GetPosition();
			renderCamera.ClipSpace = GetProjectionMatrix() * GetViewMatrix();
		}
	}
}

void EditorCameraComponent::UpdateFocus(GameObjectComponentPtr<TransformComponent>& transfComp)
{
	if (!m_isFocusing)
	{
		return;
	}

	// Any manual look, orbit, pan or zoom takes over from the animation
	InputHandler& inputHandler = RZE().GetInputHandler();
	if (inputHandler.GetMouseState().GetButtonState(EMouseButton::MouseButton_Right) == EButtonState::ButtonState_Pressed
		|| inputHandler.GetMouseState().GetButtonState(EMouseButton::MouseButton_Middle) == EButtonState::ButtonState_Pressed
		|| inputHandler.GetMouseState().CurWheelVal != 0)
	{
		m_isFocusing = false;
		return;
	}

	m_focusElapsed += static_cast<float>(RZE().GetDeltaTime());
	const float t = MathUtils::Clampf(m_focusElapsed / k_focusDuration, 0.0f, 1.0f);
	const float eased = MathUtils::SmoothStep(t);
	const float rotateEased = MathUtils::SmoothStep(MathUtils::Clampf(m_focusElapsed / k_focusRotateDuration, 0.0f, 1.0f));

	transfComp->GetPosition() = VectorUtils::Lerp(m_focusStartPos, m_focusTargetPos, eased);
	SetForward(VectorUtils::Slerp(m_focusStartForward, m_focusTargetForward, rotateEased));

	if (t >= 1.0f)
	{
		m_isFocusing = false;
	}
}

void EditorCameraComponent::GenerateCameraMatrices(const Vector3D& position)
{
	OPTICK_EVENT("GenerateCameraMatrices");

	m_projectionMat = Matrix4x4::CreatePerspectiveMatrixZeroToOne(m_fov, m_aspectRatio, m_nearCull, m_farCull);
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
}

void EditorCameraComponent::MouseInput(GameObjectComponentPtr<TransformComponent>& transfComp)
{
	InputHandler& inputHandler = RZE().GetInputHandler();

	const Vector2D& curPos = inputHandler.GetMouseState().CurPosition;
	const Vector2D diff = curPos - m_mousePrevPos;
	m_mousePrevPos = curPos;

	Vector3D& position = transfComp->GetPosition();

	Int32 wheelVal = inputHandler.GetMouseState().CurWheelVal;
	if (wheelVal != 0)
	{
		// Dolly toward the pivot by a fraction of the distance; once at the minimum, keep going and carry the pivot along
		wheelVal = MathUtils::Clamp(wheelVal, -1, 1);
		const float step = std::max(m_orbitDistance * k_wheelDollyFraction, k_minOrbitDistance) * static_cast<float>(wheelVal);
		position += m_forward * step;
		m_orbitDistance = std::max(m_orbitDistance - step, k_minOrbitDistance);
	}

	if (inputHandler.GetMouseState().CurMouseBtnStates[EMouseButton::MouseButton_Right])
	{
		const bool withSensitivity = true;
		CalculateNewForward(diff, withSensitivity);
	}
	else if (inputHandler.GetMouseState().CurMouseBtnStates[EMouseButton::MouseButton_Middle] && (diff.X() != 0.0f || diff.Y() != 0.0f))
	{
		const auto& keyboardState = inputHandler.GetKeyboardState();
		const Vector3D pivot = position + m_forward * m_orbitDistance;

		if (keyboardState.IsDownThisFrame(Win32KeyCode::Shift))
		{
			// Pan: scale pixels to metres at the pivot's depth so the scene stays under the cursor
			const float viewportHeight = RZE().GetRenderEngine().GetMainView().ViewportSize.Y();
			if (viewportHeight > 0.0f)
			{
				const float metresPerPixel = 2.0f * m_orbitDistance * std::tan(m_fov * 0.5f * MathUtils::ToRadians) / viewportHeight;
				const Vector3D right = m_forward.Cross(m_upDir).Normalized();
				const Vector3D up = right.Cross(m_forward);
				position += (up * diff.Y() - right * diff.X()) * metresPerPixel;
			}
		}
		else if (keyboardState.IsDownThisFrame(Win32KeyCode::Control))
		{
			// Dolly: drag down to pull away from the pivot, up to push in
			const float newDistance = std::max(m_orbitDistance * std::exp(diff.Y() * k_dragDollyRate), k_minOrbitDistance);
			position = pivot - m_forward * newDistance;
			m_orbitDistance = newDistance;
		}
		else
		{
			// Orbit: turn the same way as RMB look, then back the camera off the pivot along the new forward
			const bool withSensitivity = true;
			CalculateNewForward(diff, withSensitivity);
			position = pivot - m_forward * m_orbitDistance;
		}
	}
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
		outDelta *= k_lookSensitivity;
	}

	m_yawPitch += outDelta;
	m_yawPitch.SetY(MathUtils::Clampf(m_yawPitch.Y(), -k_maxPitch, k_maxPitch));

	float yawInRadians = m_yawPitch.X() * MathUtils::ToRadians;
	float pitchInRadians = m_yawPitch.Y() * MathUtils::ToRadians;

	Vector3D newForward;
	newForward.SetX(std::cos(yawInRadians) * std::cos(pitchInRadians));
	newForward.SetY(-std::sin(pitchInRadians));
	newForward.SetZ(std::sin(yawInRadians) * std::cos(pitchInRadians));

	m_forward = newForward;
	m_forward.Normalize();
}
