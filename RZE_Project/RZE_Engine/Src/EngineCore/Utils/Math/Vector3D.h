#pragma once

class Vector3D
{

public:

	Vector3D();
	Vector3D(const float x, const float y, const float z);

	float X() const;
	float Y() const;
	float Z() const;

private:

	float mX;
	float mY;
	float mZ;

};
