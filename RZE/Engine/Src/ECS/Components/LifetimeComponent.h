#pragma once

struct LifetimeComponent
{
	LifetimeComponent() {};
	LifetimeComponent(float lifetimeMS)
		: LifetimeMS(lifetimeMS) {}

	//virtual void OnEditorInspect(Apollo::EntityID entityID);

	float LifetimeMS = 100000;
};
