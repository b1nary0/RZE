#pragma once

#include <vector>

#include <Utils/DebugUtils/Debug.h>

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
	U32 AddSubSystem(Args... args)
	{
		ISubSystem* subsys = new TSubSystem(std::forward<Args>(args)...);
		AssertNotNull(subsys);
		mSubSystems.push_back(subsys);

		return mSubSystems.size() - 1;
	}

	template<class TSubSystem>
	TSubSystem* GetSubSystemByIndex(U32 index)
	{
		AssertExpr(index < mSubSystems.size());
		return static_cast<TSubSystem*>(mSubSystems[index]);
	}

private:
	std::vector<ISubSystem*> mSubSystems;

};