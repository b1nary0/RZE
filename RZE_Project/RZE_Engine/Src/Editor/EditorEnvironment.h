#pragma once

class EditorWorld;

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
};