#include <StdAfx.h>
#include <RenderCore/Renderer.h>

#include <RenderCore/HardwareInterface/OpenGL.h>
#include <RenderCore/Shaders/ShaderGroup.h>

RZE_Renderer::RZE_Renderer()
{

}

void RZE_Renderer::AddRenderItem(const RenderItemProtocol& itemProtocol)
{
	mRenderList.emplace_back(itemProtocol);
}

void RZE_Renderer::Render()
{
	for (RenderItemProtocol& itemProtocol : mRenderList)
	{
		RenderSingleItem(itemProtocol);
	}
}

void RZE_Renderer::RenderSingleItem(const RenderItemProtocol& renderItem)
{
	const OpenGLRHI& openGL = OpenGLRHI::Get();

	openGL.Clear(EGLBufferBit::Color | EGLBufferBit::Depth);

	GLuint vao;
	openGL.GenVertexArrays(1, &vao);
	openGL.BindVertexArray(vao);

	// @implementation should we have this type of assumption?
	OpenGLVBO vbo;
	vbo.SetBufferData(renderItem.VertexList->data(), sizeof(renderItem.VertexList->data()) * renderItem.VertexList->size());

	openGL.EnableVertexAttributeArray(0);
	openGL.VertexAttribPointer(0, 3, EGLDataType::Float, EGLBooleanValue::False, 0, 0);

	if (renderItem.ShaderGroup)
	{
		renderItem.ShaderGroup->Use();
	}

	openGL.DrawArrays(EGLDrawMode::Triangles, 0, 3);
}
