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

	OpenGLRHI::Get().EnableCapability(EGLCapability::DepthTest);
}

void RZE_Renderer::AddRenderItem(const RenderItemProtocol& itemProtocol)
{
	mRenderList.push(itemProtocol);
}

void RZE_Renderer::Render()
{
    const OpenGLRHI& openGL = OpenGLRHI::Get();
    openGL.Clear(EGLBufferBit::Color | EGLBufferBit::Depth);

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
	// @implementation should we have this type of assumption?
	if (renderItem.ShaderGroup)
	{
		renderItem.ShaderGroup->Use();
		renderItem.ShaderGroup->SetUniformMatrix4x4("UModelViewProjection", renderItem.ModelViewProjection);
	}

	renderItem.VAO->Bind();
	
	openGL.DrawArrays(EGLDrawMode::Triangles, 0, renderItem.VertexList->size());

	renderItem.VAO->Unbind();
}
