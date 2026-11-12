#include <Graphics/MeshDraw.h>

#include <Graphics/MaterialSlot.h>

#include <cstddef>

namespace
{
	// Transform is read as the first matrix of an ObjectBufferLayout
	static_assert(offsetof(GraphicsLayouts::ObjectBufferLayout, Transform) == 0, "ObjectBufferLayout doesn't start with its transform");
	static_assert(offsetof(GraphicsLayouts::ObjectBufferLayout, InvTransform) == sizeof(Matrix4x4),
		"ObjectBufferLayout's matrices aren't side by side");
}

GraphicsLayouts::MeshInstanceLayout MeshDraw::BuildInstance() const
{
	GraphicsLayouts::MeshInstanceLayout instance;
	// Transform is the first matrix of an ObjectBufferLayout, the object's or OwnMatrices, with its inverse after it
	instance.Object = *reinterpret_cast<const GraphicsLayouts::ObjectBufferLayout*>(Transform);
	instance.Material = Slot->BuildInstanceData();
	return instance;
}
