#pragma once

#include <Utils/LiveObjectPtr.h>
#include <Utils/PrimitiveDefs.h>

class RenderObject;
class LightObject;
class ReflectionProbeObject;

// Whether a render object is expected to move once the scene is running, which decides what's kept from it. Values are
// saved by name (RenderComponent), so new ones go at the end.
enum class ERenderMobility
{
	// Placed and left: captured into the probes. It can still be moved (e.g. in the editor, or a door), and what's
	// kept from around it is made again.
	Static,
	// Moves while the scene runs (e.g. a character or a prop): lit by the probes and their reflections, but not
	// captured into them, so moving it costs them nothing
	Movable
};

// Which render objects and lights are alive, for their handles (UniqueLiveObjectPtr), whether they were destroyed
// through the handle or not (e.g. RenderScene::ClearObjects, or the scene going away). Both kinds share one
// LiveIDAllocator, and no ID is handed out twice, so a handle to a destroyed object stays invalid even if a new one
// lands at its address. Main thread only.
struct RenderSceneLifetime
{
	static bool IsAlive(U64 id);
	static U64 GetID(const RenderObject* renderObject);
	static U64 GetID(const LightObject* lightObject);
	static U64 GetID(const ReflectionProbeObject* reflectionProbe);

	// For RenderObject and LightObject: an ID alive from construction until it's released on destruction
	static U64 Acquire();
	static void Release(U64 id);
};

// The handle to an object in a RenderScene, held by whatever made it (e.g. RenderComponent), which destroys it through
// RenderScene::DestroyRenderObject / DestroyLightObject. Invalid (and equal to nullptr) once the object is destroyed.
using RenderObjectPtr = UniqueLiveObjectPtr<RenderObject, RenderSceneLifetime>;
using LightObjectPtr = UniqueLiveObjectPtr<LightObject, RenderSceneLifetime>;
using ReflectionProbeObjectPtr = UniqueLiveObjectPtr<ReflectionProbeObject, RenderSceneLifetime>;
