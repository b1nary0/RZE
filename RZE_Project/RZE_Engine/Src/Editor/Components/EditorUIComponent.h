#pragma once

#include <Game/ECS/EntityComponent.h>

#include <Utils/Math/Vector2D.h>

class EditorUIComponent : public IEntityComponent
{
public:
	EditorUIComponent();
	EditorUIComponent(const std::string& name);
	virtual ~EditorUIComponent();

	const Vector2D& GetPosition() const;
	const Vector2D& GetDimensions() const;

	void SetPosition(const Vector2D& position);
	void SetDimensions(const Vector2D& dimensions);

private:
	Vector2D mPosition;
	Vector2D mDimensions;
};