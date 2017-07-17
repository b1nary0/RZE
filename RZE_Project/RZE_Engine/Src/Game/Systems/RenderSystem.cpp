#include <StdAfx.h>
#include <Game/Systems/RenderSystem.h>

#include <Game/ECS/Entity.h>
#include <Game/Components/LightSourceComponent.h>
#include <Game/Components/MeshComponent.h>
#include <Game/Components/TransformComponent.h>

#include <RenderCore/Renderer.h>
#include <RenderCore/HardwareInterface/OpenGL.h>

#include <RenderCore/Shaders/Shader.h>
#include <RenderCore/Shaders/ShaderGroup.h>

#include <Utils/Math/Vector4D.h>

RenderSystem::RenderSystem(IEntityAdmin* const admin)
    : IEntitySystem(admin)
{
    OpenGLRHI::Get().ClearColor(0.5f, 0.5f, 0.5f, 1.0f);
}

RenderSystem::~RenderSystem()
{
}

void RenderSystem::Init()
{
    CreateTestShaderStuff();
}

void RenderSystem::Update()
{
    RZE_Renderer* const renderer = mAdmin->GetRenderer();
    AssertNotNull(renderer);

    const std::vector<IEntity*>& entityList = mAdmin->GetEntities();
    for (auto& entity : entityList)
    {
        MeshComponent* const meshComponent = static_cast<MeshComponent* const>(entity->GetComponents()[0]);
        // @todo:josh obviously this needs to go, but for now is test.
        if (!meshComponent->GetShaderGroup())
        {
            meshComponent->SetShaderGroup(mShaderGroup);
        }

        LightSourceComponent* lightComponent = nullptr;
        if (entity->GetComponents().size() == 3)
        {
            lightComponent = static_cast<LightSourceComponent*>(entity->GetComponents()[2]);
        }

        TransformComponent* const transform = static_cast<TransformComponent* const>(entity->GetComponents()[1]);
        SceneCamera& renderCam = renderer->GetSceneCamera();

        Matrix4x4 modelMat;
        modelMat.Translate(transform->GetPosition());
        modelMat.Rotate(transform->GetRotation().ToAngle(), transform->GetRotation().ToAxis());
        modelMat.Scale(transform->GetScale());

        Matrix4x4 MVP = renderCam.GetProjectionMat() * renderCam.GetViewMat() * modelMat;

        RZE_Renderer::RenderItemProtocol renderItem;
        renderItem.mShaderGroup = meshComponent->GetShaderGroup();
        renderItem.mModelMat = modelMat;
        renderItem.mProjectionMat = renderCam.GetProjectionMat();
        renderItem.mViewMat = renderCam.GetViewMat();
        renderItem.mMeshData = meshComponent->GetMeshData();

        if (lightComponent)
        {
            RZE_Renderer::LightItemProtocol lightItem;
            lightItem.mLightColor = lightComponent->GetColor();
            lightItem.mLightPos = transform->GetPosition();
            lightItem.mLightStrength = lightComponent->GetStrength();

            renderer->AddLightItem(lightItem);
        }

        if (renderer)
        {
            renderer->AddRenderItem(renderItem);
        }
    }
}

void RenderSystem::ShutDown()
{
}

void RenderSystem::CreateTestShaderStuff()
{
    File vertShaderFile("./../RZE_Engine/Assets/Shaders/VertexShader.shader");
    File fragShaderFile("./../RZE_Engine/Assets/Shaders/FragmentShader.shader");

    AssertExpr(!vertShaderFile.Empty() && !fragShaderFile.Empty());
    GFXShader* vertShader = new GFXShader(EGLShaderType::Vertex, "TestVertexShader", vertShaderFile.Content());
    vertShader->Create();
    vertShader->Compile();

    GFXShader* fragShader = new GFXShader(EGLShaderType::Fragment, "TestFragShader", fragShaderFile.Content());
    fragShader->Create();
    fragShader->Compile();

    mShaderGroup = new GFXShaderGroup("TestShaderGroup");
    mShaderGroup->AddShader(GFXShaderGroup::EShaderIndex::Vertex, vertShader);
    mShaderGroup->AddShader(GFXShaderGroup::EShaderIndex::Fragment, fragShader);

    mShaderGroup->AddUniform("UModelMat");
    mShaderGroup->AddUniform("UProjectionMat");
    mShaderGroup->AddUniform("UViewMat");
    mShaderGroup->AddUniform("UFragColor");

     mShaderGroup->AddUniform("ULightPosition");
     mShaderGroup->AddUniform("UViewPosition");
     mShaderGroup->AddUniform("ULightColor");
     mShaderGroup->AddUniform("ULightStrength");

    mShaderGroup->GenerateShaderProgram();
}
