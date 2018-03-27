#pragma once

#include <GLM/vec2.hpp>

#include <Utils/StringUtils.h>

class Vector2D
{
public:

	Vector2D();
	Vector2D(const float val);
	Vector2D(const float x, const float y);
	Vector2D(const int x, const int y);

	float X() const;
	float Y() const;

	void SetX(float newX);
	void SetY(float newY);
	void SetXY(float newX, float newY);

	const glm::vec2& GetInternalVec() const;

	inline std::string ToString() const
	{
		return std::move(StringUtils::FormatString("[X] %f [Y] %f", X(), Y()));
	}
	
	//
	// Operators
	//
public:
	Vector2D operator-(const Vector2D& other);
	bool operator!=(const Vector2D& rhs) const;

private:
	glm::vec2 mVec;

};
