#pragma once

class ISubSystem
{
	friend class SubSystemHandler;

private:
	virtual void Initialize() = 0;
	virtual void Update() = 0;
	virtual void ShutDown() = 0;

};
