#pragma once

class GameScene
{
public:
	GameScene();
	~GameScene();

public:
	virtual void Start();
	virtual void Update();
	virtual void Finish();

public:
	void Initialize();
};