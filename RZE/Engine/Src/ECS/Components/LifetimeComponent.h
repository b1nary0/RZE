#pragma once

#include <Apollo/ECS/EntityComponent.h>

struct LifetimeComponent : Apollo::Component<LifetimeComponent>
{
	LifetimeComponent() {};
	LifetimeComponent(float lifetimeMS)
		: LifetimeMS(lifetimeMS) {}

	float LifetimeMS;
};
