#pragma once

#include <Utils/Math/Vector2D.h>

class UIElement
{
public:
	UIElement() {}
	~UIElement() {}

	virtual void OnCreate() = 0;
	virtual void OnDraw() = 0;

	void SetSize(const Vector2D& newSize) { mSize = newSize; }
	const Vector2D& GetSize() { return mSize; }

	void SetPosition(const Vector2D& newPos) { mPosition = newPos; }
	const Vector2D& GetPosition() { return mPosition; }

private:
	Vector2D mSize;
	Vector2D mPosition;
};
