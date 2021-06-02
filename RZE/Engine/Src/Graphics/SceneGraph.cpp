#include <StdAfx.h>
#include <Graphics/SceneGraph.h>

#include <Game/Model3D.h>

#include <Ecs/Components/TransformComponent.h>

#include <Graphics/Material.h>
#include <Graphics/MeshGeometry.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>

#include <Diotima/Renderer.h>


RenderSceneGraph::RenderSceneGraph()
{

}

RenderSceneGraph::~RenderSceneGraph()
{

}

size_t RenderSceneGraph::AddRootNode(const RenderNodeParms& parms)
{
	const size_t retID = mRootNodes.size();

	mRootNodes.emplace_back();
	RenderNode& rootNode = mRootNodes.back();
	rootNode.Transform = parms.Transform;
	rootNode.EntityID = parms.EntityID;

	return retID;
}

void RenderSceneGraph::AddChildNode(size_t parentIndex, const RenderNodeParms& parms)
{
	AssertExpr(parentIndex < mRootNodes.size());
	RenderNode& rootNode = mRootNodes[parentIndex];

	rootNode.Children.emplace_back();
	RenderNode& childNode = rootNode.Children.back();

	childNode.Geometry = parms.GeometryData;
	childNode.RenderObjectIndex = parms.RenderObjectIndex;
	childNode.Transform = parms.Transform;
}

void RenderSceneGraph::ForEachNodeUpdate(VisitorFunc func)
{
	Apollo::EntityHandler& entityHandler = RZE_Application::RZE().GetActiveScene().GetEntityHandler();
	// #TODO
	// Root nodes don't contribute to the scene. This is because of a rushed implementation, and needs
	// to be reworked. As a result, VisitorFunc gets called with the "renderable" children. lol
	for (RenderNode& rootNode : mRootNodes)
	{
		TransformComponent* const transformComponent = entityHandler.GetComponent<TransformComponent>(rootNode.EntityID);

		// rootnodes don't have a render object
		rootNode.Transform = transformComponent->GetAsMat4x4();
		for (auto& child : rootNode.Children)
		{
			func(child.RenderObjectIndex, rootNode.Transform);
		}
	}
}
