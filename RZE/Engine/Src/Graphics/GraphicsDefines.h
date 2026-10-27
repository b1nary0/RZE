#pragma once

#include <Utils/LiveObjectPtr.h>
#include <Utils/PrimitiveDefs.h>

class RenderObject;
class LightObject;

// Which render objects and lights are alive, for their handles (UniqueLiveObjectPtr), whether they were destroyed
// through the handle or not (e.g. RenderScene::ClearObjects, or the scene going away). Both kinds share one
// LiveIDAllocator, and no ID is handed out twice, so a handle to a destroyed object stays invalid even if a new one
// lands at its address. Main thread only.
struct RenderSceneLifetime
{
	static bool IsAlive(U64 id);
	static U64 GetID(const RenderObject* renderObject);
	static U64 GetID(const LightObject* lightObject);
};

// The handle to an object in a RenderScene, held by whatever made it (e.g. RenderComponent), which destroys it through
// RenderScene::DestroyRenderObject / DestroyLightObject. Invalid (and equal to nullptr) once the object is destroyed.
using RenderObjectPtr = UniqueLiveObjectPtr<RenderObject, RenderSceneLifetime>;
using LightObjectPtr = UniqueLiveObjectPtr<LightObject, RenderSceneLifetime>;
