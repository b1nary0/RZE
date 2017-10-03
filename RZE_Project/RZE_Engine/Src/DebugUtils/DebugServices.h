#pragma once

class DebugServices
{
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

private:
	DebugServices();
};
