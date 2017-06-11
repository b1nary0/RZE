#include <StdAfx.h>
#include <RenderCore/Renderer.h>

#include <RenderCore/HardwareInterface/OpenGL.h>
#include <RenderCore/Shaders/ShaderGroup.h>

RZE_Renderer::RZE_Renderer()
{
	SceneCameraProps cameraProps;
	cameraProps.Position = Vector3D(0.0f, 0.0f, 0.0f);
	cameraProps.Direction = Vector3D(0.0f, 0.0f, -1.0f);
	cameraProps.UpDir = Vector3D(0.0f, 1.0f, 0.0f);
	cameraProps.FOV = 45.0f;
	cameraProps.AspectRatio = (1280.0f / 720.0f);
	cameraProps.NearCull = 0.1f;
	cameraProps.FarCull = 1000.0f;

	mSceneCamera = new SceneCamera(cameraProps);
	mSceneCamera->GenerateProjectionMat();
	mSceneCamera->GenerateViewMat();
}

void RZE_Renderer::AddRenderItem(const RenderItemProtocol& itemProtocol)
{
	mRenderList.push(itemProtocol);
}

void RZE_Renderer::Render()
{
	while (!mRenderList.empty())
	{
		RenderItemProtocol& itemProtocol = mRenderList.front();
		mRenderList.pop();

		RenderSingleItem(itemProtocol);
	}
}

SceneCamera& RZE_Renderer::GetSceneCamera()
{
	return *mSceneCamera;
}

void RZE_Renderer::RenderSingleItem(RenderItemProtocol& renderItem)
{
	const OpenGLRHI& openGL = OpenGLRHI::Get();

	openGL.Clear(EGLBufferBit::Color | EGLBufferBit::Depth);

	// @implementation should we have this type of assumption?
	renderItem.VBO->SetBufferData(renderItem.VertexList->data(), sizeof(renderItem.VertexList->data()) * renderItem.VertexList->size());

	if (renderItem.ShaderGroup)
	{
		renderItem.ShaderGroup->SetUniformMatrix4x4("UModelViewProjection", renderItem.ModelViewProjection);
		renderItem.ShaderGroup->Use();
	}

	openGL.BindVertexArray(renderItem.VAO);
	openGL.EnableVertexAttributeArray(0);
	openGL.VertexAttribPointer(0, 3, EGLDataType::Float, EGLBooleanValue::False, 0, 0);
	openGL.DrawArrays(EGLDrawMode::Triangles, 0, 3);
	openGL.BindVertexArray(0);
}
