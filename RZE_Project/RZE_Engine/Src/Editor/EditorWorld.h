#pragma once

#include <Game/ECS/EntityAdmin.h>

class EditorWorld : public IEntityAdmin
{
public:
	EditorWorld(RZE_Renderer* const renderer);
	virtual ~EditorWorld();

	virtual RZE_Renderer* const GetRenderer() const override;

	virtual void Init() override;
	virtual void Update() override;
	virtual void ShutDown() override;

};