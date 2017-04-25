#pragma once

class GameWorld
{
	// @todo take a look at this connection
	friend class RZE_Engine;

public:
	GameWorld();
	~GameWorld();

	void Update();

private:
	// @implementation a list of systems to be updated and controlled

};
