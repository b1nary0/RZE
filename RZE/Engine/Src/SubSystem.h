#pragma once

class ISubSystem
{
	friend class RZE_Engine;

private:
	virtual void Initialize() = 0;
	virtual void Update() = 0;
	virtual void ShutDown() = 0;

};
