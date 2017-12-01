#pragma once

 #include <Apollo/ECS/EntityAdmin.h>
 
 class GameWorld : public IEntityAdmin
 {
 
 public:
 	GameWorld();
 	virtual ~GameWorld();
 
 	virtual void Init() override;
 	virtual void Update() override;
 	virtual void ShutDown() override;
 
 private:
 	void InitSystems();
 
 private:
 };
