#pragma once

#include <Apollo/EntityHandler.h>

class GameScene
{
public:
	GameScene();
	~GameScene();

public:
	virtual void Start();
	virtual void Update();
	virtual void Finish();

	Apollo::EntityHandler& GetEntityHandler() { return mEntityHandler; }

	void Initialize();

private:

	Apollo::EntityHandler mEntityHandler;
};