#include <StdAfx.h>
#include <RenderCore/Renderer.h>

#include <RenderCore/Graphics/Mesh.h>
#include <RenderCore/HardwareInterface/OpenGL.h>
#include <RenderCore/Shaders/ShaderGroup.h>

#include <Utils/Math/Vector4D.h>

RZE_Renderer::RZE_Renderer()
{
    WindowSettings& windowSettings = RZE_Engine::Get()->GetWindowSettings();

    SceneCameraProps cameraProps;
    cameraProps.Position = Vector3D(0.0f, 5.0f, 10.0f); 
    cameraProps.Direction = Vector3D(0.0f, -2.5f, -10.0f);
    cameraProps.UpDir = Vector3D(0.0f, 1.0f, 0.0f);
    cameraProps.FOV = 45.0f;
    cameraProps.AspectRatio = windowSettings.GetDimensions().X() / windowSettings.GetDimensions().Y();
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

void RZE_Renderer::AddLightItem(const LightItemProtocol& itemProtocol)
{
    mLightingList.push_back(itemProtocol);
}

void RZE_Renderer::Render()
{
    const OpenGLRHI& openGL = OpenGLRHI::Get();
    openGL.Clear(EGLBufferBit::Color | EGLBufferBit::Depth);

    while (!mRenderList.empty())
    {
        RenderItemProtocol& itemProtocol = mRenderList.front();
        RenderSingleItem(itemProtocol);
        mRenderList.pop();
    }

    mLightingList.clear();
}

SceneCamera& RZE_Renderer::GetSceneCamera()
{
    AssertNotNull(mSceneCamera);
    return *mSceneCamera;
}

void RZE_Renderer::RenderSingleItem(RenderItemProtocol& renderItem)
{
    const OpenGLRHI& openGL = OpenGLRHI::Get();
    // @implementation should we have this type of assumption?
    if (renderItem.mShaderGroup)
    {
        renderItem.mShaderGroup->Use();        
        
        renderItem.mShaderGroup->SetUniformMatrix4x4("UModelMat", renderItem.mModelMat);
        renderItem.mShaderGroup->SetUniformMatrix4x4("UProjectionMat", renderItem.mProjectionMat);
        renderItem.mShaderGroup->SetUniformMatrix4x4("UViewMat", renderItem.mViewMat);

        renderItem.mShaderGroup->SetUniformVector4D("UFragColor", Vector4D(0.25f, 0.25f, 0.25f, 1.0f));

        for (auto& light : mLightingList)
        {
            renderItem.mShaderGroup->SetUniformVector3D("ULightPosition", light.mLightPos);
            renderItem.mShaderGroup->SetUniformVector3D("UViewPosition", mSceneCamera->GetPositionVec());
            renderItem.mShaderGroup->SetUniformVector3D("ULightColor", light.mLightColor);
            renderItem.mShaderGroup->SetUniformFloat("ULightStrength", light.mLightStrength);
        }
    }

    const std::vector<GFXMesh*>& meshList = renderItem.mMeshData->GetMeshList();
    for (auto& mesh : meshList)
    {
        mesh->GetVAO()->Bind();

        OpenGLRHI::Get().DrawElements(EGLDrawMode::Triangles, mesh->GetIndices().size(), EGLDataType::UnsignedInt, nullptr);

        mesh->GetVAO()->Unbind();
    }
}

RZE_Renderer::RenderItemProtocol::RenderItemProtocol()
{
    mShaderGroup        = nullptr;
    mMeshData           = nullptr;
}
