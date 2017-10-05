#pragma once

#include <EngineUI/Windows/LogWindow.h>

class DebugServices
{
	friend class RZE_Engine;

	static DebugServices* sInstance;

public:
	~DebugServices();

	static DebugServices& Get()
	{
		if (!sInstance)
		{
			sInstance = new DebugServices();
		}

		return *sInstance;
	}

	void LogDebug(const std::string& msg);
	void LogInfo(const std::string& msg);
	void LogWarning(const std::string& msg);
	void LogError(const std::string& msg);

private:
	DebugServices();

	LogWindow mLogWindow;
};
