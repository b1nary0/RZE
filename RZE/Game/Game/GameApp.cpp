#include "GameApp.h"

#include <RZE_Config.h>
#include <RZE.h>

#include <Game/GameWorld.h>

#include <DebugUtils/DebugServices.h>

#include <Apollo/EntityComponentSystem.h>

#include <Diotima/Graphics/Mesh.h>
#include <Diotima/Graphics/Texture2D.h>
#include <Diotima/Shaders/ShaderGroup.h>

#include <Windowing/WinKeyCodes.h>

#include <Utils/Math/Math.h>
#include <Utils/DebugUtils/Debug.h>


// TEST
#include <Ecs/Components/CameraComponent.h>
#include <ECS/Components/LightSourceComponent.h>
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/TransformComponent.h>

#include <Game/Systems/RotateSystem.h>

using namespace Diotima;

// Test stuff -- this stuff will likely be removed at some point
// or if it ends up being "permanent", implemented more sensibly.
static Vector3D cameraStartPos = Vector3D(0.0f, 1.0f, 15.0f);
static bool mIsCameraStartSequence = true;

GameApp::GameApp()
	: RZE_Game()
{
}

GameApp::~GameApp()
{
}

void GameApp::Start()
{
	RZE_Game::Start();

	// ALL TEST CODE

	Apollo::ComponentHandler& componentHandler = RZE_Engine::Get()->GetComponentHandler();

	componentHandler.AddSystem<RotateSystem>();

	// Leaving the for loop for testing purposes
	for (int i = 0; i < 1; ++i)
	{
		Apollo::EntityID entity = componentHandler.CreateEntity();

		componentHandler.AddComponent<MeshComponent>(entity, "./../Engine/Assets/3D/Lamp.obj");
		componentHandler.AddComponent<TransformComponent>(entity);
	}

	Apollo::EntityID entity = componentHandler.CreateEntity();
	componentHandler.AddComponent<LightSourceComponent>(entity, Vector3D(1.0f, 0.0f, 0.0f), 1.0f);
	componentHandler.AddComponent<TransformComponent>(entity, Vector3D(0.0f, 0.5f, 2.0f));

	Apollo::EntityID camera = componentHandler.CreateEntity();
	componentHandler.AddComponent<CameraComponent>(entity);

	// 	const char* const cubeFilePath = "./../Engine/Assets/3D/Cube.obj";
	// 	const char* const miniCooperFilePath = "./../Engine/Assets/3D/MiniCooper.obj";
	// 	const char* const lampFilePath = "./../Engine/Assets/3D/Lamp.obj";
	// 
	// 	const char* const quadMeshFilePath = "./../Engine/Assets/3D/Quad.obj";
	// 	const char* const quadTextureFilePath = "./../Engine/Assets/2D/Container.jpg";

	// 	ResourceHandler& resourceHandler = RZE_Engine::Get()->GetResourceHandler();
	// 
	// 	ResourceHandle cubeMesh = resourceHandler.RequestResource<MeshResource>(cubeFilePath);
	// 	ResourceHandle lampMesh = resourceHandler.RequestResource<MeshResource>(lampFilePath);
	// 
	// 	ResourceHandle quadMesh = resourceHandler.RequestResource<MeshResource>(quadMeshFilePath);
	// 	ResourceHandle quadTex = resourceHandler.RequestResource<GFXTexture2D>(quadTextureFilePath);
}

void GameApp::Update()
{
	RZE_Game::Update();
}

// void GameApp::CreateTextureQuad(const ResourceHandle& meshHandle, const ResourceHandle& textureHandle)
// {
// 	GameEntity* entity = RZE_Engine::Get()->GetECS()->AddEntity<GameEntity>("TextureQuad");
// 
// 	MeshComponent* const meshComp = entity->AddComponent<MeshComponent>();
// 	meshComp->SetMeshHandle(meshHandle);
// 	meshComp->SetTextureHandle(textureHandle);
// 	meshComp->SetShaderGroup(mTextureShader);
// 
// 	TransformComponent* const transfComp = entity->AddComponent<TransformComponent>();
// 	transfComp->SetPosition(Vector3D(0.0f, 2.0f, -3.0f));
// 	transfComp->SetScale(Vector3D(4.0f, 3.0f, 0.0f));
// 
// 	mEntities.push_back(entity);
// }
// 
//
// 
// void GameApp::CreateTextureShader()
// {
// 	const char* const vertShaderFilePath = "./../Engine/Assets/Shaders/TextureVert.shader";
// 	const char* const fragShaderFilePath = "./../Engine/Assets/Shaders/TextureFrag.shader";
// 
// 	ResourceHandle vertShaderHandle = RZE_Engine::Get()->GetResourceHandler().RequestResource<GFXShader>(vertShaderFilePath, EGLShaderType::Vertex, "TextureVertShader");
// 	ResourceHandle fragShaderHandle = RZE_Engine::Get()->GetResourceHandler().RequestResource<GFXShader>(fragShaderFilePath, EGLShaderType::Fragment, "TextureFragShader");
// 
// 	GFXShader* vertShader = RZE_Engine::Get()->GetResourceHandler().GetResource<GFXShader>(vertShaderHandle);
// 	vertShader->Create();
// 	vertShader->Compile();
// 
// 	GFXShader* fragShader = RZE_Engine::Get()->GetResourceHandler().GetResource<GFXShader>(fragShaderHandle);
// 	fragShader->Create();
// 	fragShader->Compile();
// 
// 	mTextureShader = new GFXShaderGroup("TextureShader");
// 	mTextureShader->AddShader(GFXShaderGroup::EShaderIndex::Vertex, vertShader);
// 	mTextureShader->AddShader(GFXShaderGroup::EShaderIndex::Fragment, fragShader);
// 
// 	mTextureShader->AddUniform("UModelMat");
// 	mTextureShader->AddUniform("UProjectionMat");
// 	mTextureShader->AddUniform("UViewMat");
// 
// 	mTextureShader->AddUniform("ULightPosition");
// 	mTextureShader->AddUniform("UViewPosition");
// 	mTextureShader->AddUniform("ULightColor");
// 	mTextureShader->AddUniform("ULightStrength");
// 
// 	mTextureShader->AddUniform("UFragColor");
// 	//mTextureShader->AddUniform("UTexture2D");
// 
// 	mTextureShader->GenerateShaderProgram();
// 
// 	RZE_Engine::Get()->GetResourceHandler().ReleaseResource(vertShaderHandle);
// 	RZE_Engine::Get()->GetResourceHandler().ReleaseResource(fragShaderHandle);
// }
