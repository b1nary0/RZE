#include <StdAfx.h>
#include <Game/Systems/RenderSystem.h>

#include <Game/ECS/Entity.h>
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

        TransformComponent* const transform = static_cast<TransformComponent* const>(entity->GetComponents()[1]);
        SceneCamera& renderCam = renderer->GetSceneCamera();

        Matrix4x4 modelMat;
        modelMat.Translate(transform->GetPosition());
        modelMat.Rotate(transform->GetRotation().ToAngle(), transform->GetRotation().ToAxis());
        modelMat.Scale(transform->GetScale());

        Matrix4x4 MVP = renderCam.GetProjectionMat() * renderCam.GetViewMat() * modelMat;

        RZE_Renderer::RenderItemProtocol renderItem;
        renderItem.VertexList = &(meshComponent->GetVertexList());
        renderItem.ShaderGroup = meshComponent->GetShaderGroup();
        renderItem.ModelViewProjection = MVP;
        renderItem.VAO = meshComponent->GetVAO();
        renderItem.VBO = meshComponent->GetVBO();

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
    mShaderGroup->AddShader(EGFXShaderType::Vertex, vertShader);
    mShaderGroup->AddShader(EGFXShaderType::Fragment, fragShader);

    mShaderGroup->AddUniform("UModelViewProjection");
    //mShaderGroup->AddUniform("UFragColor");

    mShaderGroup->GenerateShaderProgram();
}
