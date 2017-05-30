#pragma once

#include <vector>

class GFXShaderGroup;

class RZE_Renderer
{
public:
	typedef struct RenderItemProtocol
	{
		std::vector<float>* VertexList;
		GFXShaderGroup* ShaderGroup;
	} RenderItemProtocol;

public:
	RZE_Renderer();

	void AddRenderItem(const RenderItemProtocol& itemProtocol);
	void Render();

private:
	void RenderSingleItem(const RenderItemProtocol& itemProtocol);

private:
	std::vector<RenderItemProtocol> mRenderList;

};
