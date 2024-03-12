#include <StdAfx.h>
#include <Game/World/GameObjectComponents/PlayerControllerComponent.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Utils/Math/Math.h>

void PlayerControllerComponent::Update()
{
	InputHandler& inputHandler = RZE().GetInputHandler();

	float dt = static_cast<float>(RZE().GetDeltaTime());
	float speedDelta = m_speed * dt;
	bool hasValidMovementInput = false;

	GameObjectComponentPtr<TransformComponent> transfComp = GetOwner()->GetComponent<TransformComponent>();

	if (inputHandler.GetKeyboardState().GetButtonState(Win32KeyCode::Key_W) == EButtonState::ButtonState_Pressed
		|| inputHandler.GetKeyboardState().GetButtonState(Win32KeyCode::Key_W) == EButtonState::ButtonState_Hold)
	{
		transfComp->SetPosition(transfComp->GetPosition() + m_forward * speedDelta);
		hasValidMovementInput = true;
	}
	else if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_S])
	{
		transfComp->SetPosition(transfComp->GetPosition() - m_forward * speedDelta);
		hasValidMovementInput = true;
	}

	if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_A])
	{
		transfComp->SetPosition(transfComp->GetPosition() - m_forward.Cross(m_upDir).Normalize() * speedDelta);
		hasValidMovementInput = true;
	}
	else if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_D])
	{
		transfComp->SetPosition(transfComp->GetPosition() + m_forward.Cross(m_upDir).Normalize() * speedDelta);
		hasValidMovementInput = true;
	}

	if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_1])
	{
		// Focus object
		hasValidMovementInput = true;
	}

	// Mouse Input
	{
		const Vector3D& curPos = inputHandler.GetMouseState().CurPosition;

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

		m_mousePrevPos = curPos;
	}
}
