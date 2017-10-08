#pragma once

class EditorWorld;
class EditorEntity;

class EditorEnvironment
{
public:
	EditorEnvironment();
	~EditorEnvironment();

	void Init();
	void Update();
	void Shutdown();

	void PostToLogWindow(const std::string& msg);

private:
	void InternalInit();

private:
	EditorWorld* mWorld;

	EditorEntity* mLogWindowEnt;
};