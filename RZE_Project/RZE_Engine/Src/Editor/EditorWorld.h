#pragma once

#include <Game/ECS/EntityAdmin.h>

class EditorWorld : public IEntityAdmin
{
public:
	EditorWorld();
	virtual ~EditorWorld();

	virtual RZE_Renderer* const GetRenderer() const override;

	virtual void Init() override;
	virtual void Update() override;
	virtual void ShutDown() override;

private:
	// #TODO(Josh) This should DEFINITELY be removed ASAP along with the GetRenderer call (or at least NOT a part of the IEntityAdmin interface
	virtual void InternalSetRenderer(RZE_Renderer* const renderer) override {}
};