#pragma once

#include <Game/World/GameObject/Properties/PropertyDescriptor.h>

#include <vector>

class StaticMeshInstance;

// Property helpers for components that draw a mesh (RenderComponent, GifChatComponent), so they show it the same way.
// Components describe what they show; the mesh types themselves don't describe properties.

// Each of the instance's material slots, as the material it draws with, labelled with the slot's name
std::vector<ObjectRef> MakeMaterialSlotRefs(const StaticMeshInstance& meshInstance);
