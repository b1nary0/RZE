#pragma once

#include <EngineCore/Resources/TypedResourceHandle.h>

#include <Graphics/DynamicVertexBuffer.h>
#include <Graphics/RenderData/DisplayColourData.h>
#include <Graphics/RenderStage.h>

class VertexShader;
class PixelShader;

class DebugDrawRenderStage : public IRenderStage
{
public:
	DebugDrawRenderStage() = default;

	~DebugDrawRenderStage() override = default;

	const char* GetName() const override { return "DebugDrawRenderStage"; }

	void Initialize() override;
	void Setup(RenderStageBuilder& builder) override;
	void Render(RenderContext& context) override;

private:
	// Matches the Vertex_Line input layout: POSITION @ 0, COLOUR @ 12, FADECENTRE @ 28, FADERANGE @ 40
	struct LineVertex
	{
		float position[3];
		float colour[4];
		float fadeCentre[3];
		float fadeRange[2];
	};
	static_assert(sizeof(LineVertex) == 48, "LineVertex must match the Vertex_Line input layout");

private:
	// Lines are drawn into it after tonemapping, depth-tested against the scene
	RenderInOut<DisplayColourData> m_displayColour;

	// @TODO temp until ShaderTechniques are properly implemented
// (could be a while)
	TypedResourceHandle<VertexShader> m_vertexShader;

	TypedResourceHandle<PixelShader> m_lineShader;

	// All lines for a Render() call are batched into this one buffer
	DynamicVertexBuffer m_lineBuffer;
	// The lines drawn again for their parts behind the scene (DebugLine::occludedAlpha)
	DynamicVertexBuffer m_occludedLineBuffer;
	// Reused every call so steady state does no heap allocation
	std::vector<LineVertex> m_scratchVertices;
	std::vector<LineVertex> m_scratchOccludedVertices;
};
