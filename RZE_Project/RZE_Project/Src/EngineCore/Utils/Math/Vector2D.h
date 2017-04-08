#pragma once

class Vector2D
{
public:

	Vector2D();
	Vector2D(const float x, const float y);

	float X() const;
	float Y() const;

private:

	float mX;
	float mY;

};
