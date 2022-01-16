#pragma once

#include <Utils/PrimitiveDefs.h>
#include <Utils/Math/Matrix4x4.h>

class MeshGeometry;

// #TODO
// This is currently just a direct adaptation from the code previously in RenderSystem.
// This is the first move of many to move responsibility out of RenderSystem and into its
// constituent parts; meaning it's not really a graph so long as this comment exists. It's
// some weird, bastard type shit. I expect this class to look 100% different after it matures.
class RenderSceneGraph
{
private:
	struct RenderNode
	{
		U32 RenderObjectIndex;
		//Apollo::EntityID EntityID;
		const MeshGeometry* Geometry = nullptr;
		Matrix4x4 Transform = Matrix4x4::IDENTITY;
		std::vector<RenderNode> Children;
	};

public:
	typedef Functor<void, U32, const Matrix4x4&> VisitorFunc;

	struct RenderNodeParms
	{
		U32 RenderObjectIndex;
		//Apollo::EntityID EntityID;
		const MeshGeometry* GeometryData;
		Matrix4x4 Transform = Matrix4x4::IDENTITY;
	};

public:
	RenderSceneGraph();
	~RenderSceneGraph();

public:
	size_t AddRootNode(const RenderNodeParms& parms);
	void AddChildNode(size_t parentIndex, const RenderNodeParms& parms);

	// #TODO
	// Not implementing these functions has broken deleting entities (and maybe more?)
	// Implement ASAP
	void RemoveNode();

	// #TODO
	// This function needs to die almost immediately. This is entirely a workaround
	// to easily and simply keep things working while decoupling as much as possible.
	// Turns out, needs more thought. I'm this far already with no time so just contriving
	// some ugly temp solution (i.e all this code ever).
	// Essentially this is doing an update on the root RenderNodes by applying a transform.
	// This just feels absolutely awful so it needs a better solution. It can't be relied on at all
	// to support future code.
	void ForEachNodeUpdate(VisitorFunc func);

private:
	std::vector<RenderNode> mRootNodes;
};