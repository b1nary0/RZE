#pragma once

#include <Graphics/DynamicVertexBuffer.h>
#include <Graphics/RenderStage.h>

class VertexShader;
class PixelShader;

class DebugDrawRenderStage : public IRenderStage
{
public:
	DebugDrawRenderStage() = default;

	~DebugDrawRenderStage() override = default;

	void Initialize() override;
	void Update(const RenderCamera& camera, const RenderEngine::SceneData& renderData) override {}
	void Render(const RenderCamera& camera, const RenderEngine::SceneData& renderData) override;

	// @todo make an enum for this so we can add and subtract from other stage placements
	U32 GetPriority() override { return 50; }

private:
	// Matches the Vertex_Line input layout: POSITION @ 0, COLOUR @ 12
	struct LineVertex
	{
		float position[3];
		float colour[3];
	};
	static_assert(sizeof(LineVertex) == 24, "LineVertex must match the Vertex_Line input layout");

private:
	// @TODO temp until ShaderTechniques are properly implemented
// (could be a while)
	ResourceHandle m_vertexShaderResource;
	const VertexShader* m_vertexShader = nullptr; // @TODO This is just to avoid having to GetResource() in a loop but is more a deficiency of the Resource API

	ResourceHandle m_lineShaderResource;
	const PixelShader* m_lineShader = nullptr;

	// All lines for a Render() call are batched into this one buffer
	DynamicVertexBuffer m_lineBuffer;
	// Reused every call so steady state does no heap allocation
	std::vector<LineVertex> m_scratchVertices;
};
