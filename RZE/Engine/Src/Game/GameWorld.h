#pragma once

 #include <Apollo/ECS/EntityAdmin.h>
 
 class GameWorld : public Apollo::EntityAdmin
 {
 
 public:
 	GameWorld();
 	virtual ~GameWorld();
 
 	virtual void Initialize() override;
 	virtual void Update() override;
 	virtual void ShutDown() override;
 
 private:
 	void InitSystems();
 
 private:
 };
