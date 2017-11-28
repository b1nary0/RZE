#pragma once

#include <vector>

#include <DebugUtils/Debug.h>

#include <Utils/Interfaces/SubSystem.h>

class SubSystemHandler
{
public:
	SubSystemHandler();
	~SubSystemHandler();

	void InitializeSubSystems();
	void UpdateSubSystems();
	void ShutDownSubSystems();

	template<class TSubSystem, typename... Args>
	TSubSystem* AddSubSystem(Args... args)
	{
		ISubSystem* subsys = new TSubSystem(std::forward<Args>(args)...);
		AssertNotNull(subsys);
		mSubSystems.push_back(subsys);

		return static_cast<TSubSystem*>(subsys);
	}

private:
	std::vector<ISubSystem*> mSubSystems;

};