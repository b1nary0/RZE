#pragma once

#include <memory>

class RZE_Application
{

public:

	RZE_Application();
	~RZE_Application();

	void Start();

	bool IsRunning() const;

	void Update();

protected:

	void Init();

	std::unique_ptr<class RZE_EngineCore> mEngineCore;

	std::weak_ptr<class Win32Window> mWindow;

	bool bIsRunning;
};
