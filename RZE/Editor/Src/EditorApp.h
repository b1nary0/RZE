#pragma once

#include <RZE.h>

class Editor : public RZE_Game
{
public:
	Editor() {}
	virtual ~Editor() {}

	virtual void Start()
	{
		RZE_Game::Start();
	}
	virtual void Update()
	{
		RZE_Game::Update();
	}
};