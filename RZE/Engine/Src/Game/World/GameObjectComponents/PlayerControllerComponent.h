#pragma once

#include <Game/World/GameObject/GameObjectComponent.h>

class PlayerControllerComponent : public GameObjectComponent<PlayerControllerComponent>
{
public:
	PlayerControllerComponent() = default;
	~PlayerControllerComponent() = default;

public:
	void Update() override;

private:
	float m_speed = 5.0f;
	Vector3D m_mousePrevPos;
	Vector3D m_yawPitchRoll;
	Vector3D m_upDir{ 0.0f, 1.0f, 0.0f };
	Vector3D m_forward{ 0.0f, 0.0f, -1.0f };
};