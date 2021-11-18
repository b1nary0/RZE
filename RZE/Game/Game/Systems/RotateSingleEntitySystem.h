#pragma once

#include <Apollo/ECS/EntitySystem.h>

#include <EngineCore/Resources/ResourceHandler.h>

// #NOTE
// I keep coming back to rotating an entity for scenes, 
// and using TestbedSystem where I do that, losing some code
// so HARNESSING HTE POWA of Apollo::EntitySystem to pull that code into it's
// own "feature". Probably wouldn't normally make something at this granularity...
class RotateSingleEntitySystem : public Apollo::EntitySystem
{
public:
	RotateSingleEntitySystem(Apollo::EntityHandler* const entityHandler)
		: Apollo::EntitySystem(entityHandler) {}

public:
	void Initialize() override;
	void Update(const std::vector<Apollo::EntityID>& entities) override;
	void ShutDown() override;

private:
	Apollo::EntityID mRenderEntity = Apollo::kInvalidEntityID;
};