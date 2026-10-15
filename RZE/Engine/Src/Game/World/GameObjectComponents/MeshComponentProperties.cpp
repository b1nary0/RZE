#include <StdAfx.h>
#include <Game/World/GameObjectComponents/MeshComponentProperties.h>

#include <Game/World/GameObject/Properties/PropertyBuilder.h>

#include <Graphics/Material.h>
#include <Graphics/MaterialSlot.h>
#include <Graphics/StaticMeshInstance.h>

std::vector<ObjectRef> MakeMaterialSlotRefs(const StaticMeshInstance& meshInstance)
{
	std::vector<ObjectRef> refs;
	refs.reserve(meshInstance.GetMaterialSlots().size());
	for (const MaterialSlot& slot : meshInstance.GetMaterialSlots())
	{
		refs.push_back(MakeObjectRef(&slot.GetMaterial(), slot.GetName()));
	}
	return refs;
}
