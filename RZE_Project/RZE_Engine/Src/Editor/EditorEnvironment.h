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

private:
	void InternalInit();

private:
	EditorWorld* mWorld;

	EditorEntity* mLogWindowEnt;
};