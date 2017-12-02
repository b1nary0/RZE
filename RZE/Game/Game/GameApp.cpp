#include "GameApp.h"

#include <RZE_Config.h>
#include <RZE.h>

#include <Game/GameEntity.h>
#include <Game/GameWorld.h>
#include <Components/MeshComponent.h>
#include <Components/TransformComponent.h>
#include <Components/LightSourceComponent.h>

#include <Systems/EntityRenderSystem.h>

#include <DebugUtils/DebugServices.h>

#include <Apollo/EntityComponentSystem.h>

#include <Diotima/Graphics/Mesh.h>
#include <Diotima/Graphics/Texture2D.h>
#include <Diotima/Shaders/ShaderGroup.h>

#include <Windowing/WinKeyCodes.h>

#include <Utils/Math/Math.h>
#include <Utils/DebugUtils/Debug.h>

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
	
// 	CreateDefaultShader();
// 	CreateTextureShader();

	const char* const cubeFilePath = "./../Engine/Assets/3D/Cube.obj";
	const char* const miniCooperFilePath = "./../Engine/Assets/3D/MiniCooper.obj";
	const char* const lampFilePath = "./../Engine/Assets/3D/Lamp.obj";

	const char* const quadMeshFilePath = "./../Engine/Assets/3D/Quad.obj";
	const char* const quadTextureFilePath = "./../Engine/Assets/2D/Container.jpg";

	ResourceHandler& resourceHandler = RZE_Engine::Get()->GetResourceHandler();

	ResourceHandle cubeMesh = resourceHandler.RequestResource<MeshResource>(cubeFilePath);
	ResourceHandle lampMesh = resourceHandler.RequestResource<MeshResource>(lampFilePath);

	ResourceHandle quadMesh = resourceHandler.RequestResource<MeshResource>(quadMeshFilePath);
	ResourceHandle quadTex = resourceHandler.RequestResource<GFXTexture2D>(quadTextureFilePath);

// 	CreateLight(cubeMesh);
// 	CreateGround(cubeMesh);
// 	CreateLampObjects(lampMesh);
// 	CreateTextureQuad(quadMesh, quadTex);
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
// void GameApp::CreateLight(const ResourceHandle& resourceHandle)
// {
// 	mLightEntity = RZE_Engine::Get()->GetECS()->AddEntity<GameEntity>();
// 
// 	TransformComponent* lightTransform = mLightEntity->AddComponent<TransformComponent>();
// 	lightTransform->SetPosition(Vector3D(-8.0f, 15.0f, -2.0f));
// 
// 	LightSourceComponent* lightComponent = mLightEntity->AddComponent<LightSourceComponent>("MainTestLight");
// 	lightComponent->SetColor(Vector3D(1.0f, 1.0f, 1.0f));
// 	lightComponent->SetStrength(0.75f);
// 
// 	mEntities.push_back(mLightEntity);
// }
// 
// void GameApp::CreateGround(const ResourceHandle& resourceHandle)
// {
// 	GameEntity* groundEntity = RZE_Engine::Get()->GetECS()->AddEntity<GameEntity>();
// 
// 	MeshComponent* const meshComponent = groundEntity->AddComponent<MeshComponent>();
// 	meshComponent->SetMeshHandle(resourceHandle);
// 	meshComponent->SetShaderGroup(mDefaultShader);
// 
// 	TransformComponent* const transformComp = groundEntity->AddComponent<TransformComponent>();
// 	transformComp->SetPosition(Vector3D(-10.0f, -5.0f, -20.0f));
// 	transformComp->SetRotation(Quaternion(Vector3D(0.0f, 0.0f, 0.0f)));
// 	transformComp->SetScale(Vector3D(20.0f, 0.5f, 20.0f));
// 
// 	mEntities.push_back(groundEntity);
// }
// 
// void GameApp::CreateLampObjects(const ResourceHandle& resourceHandle)
// {
// 
// 	//
// 	// 1 - NE
// 	//
// 	GameEntity* lampEntity = RZE_Engine::Get()->GetECS()->AddEntity<GameEntity>();
// 
// 	MeshComponent* meshComponent = lampEntity->AddComponent<MeshComponent>("LampMesh");
// 	meshComponent->SetMeshHandle(resourceHandle);
// 	meshComponent->SetShaderGroup(mDefaultShader);
// 
// 	TransformComponent* transformComp = lampEntity->AddComponent<TransformComponent>();
// 	transformComp->SetPosition(Vector3D(-8.0f, -3.0f, -15.0f));
// 	transformComp->SetScale(Vector3D(0.5f, 0.5f, 0.5f));
// 
// 	mEntities.push_back(lampEntity);
// 
// 	//
// 	// 2 - NW
// 	//
// 	lampEntity = RZE_Engine::Get()->GetECS()->AddEntity<GameEntity>();
// 
// 	meshComponent = lampEntity->AddComponent<MeshComponent>("LampMesh");
// 	meshComponent->SetMeshHandle(resourceHandle);
// 	meshComponent->SetShaderGroup(mDefaultShader);
// 
// 	transformComp = lampEntity->AddComponent<TransformComponent>("LampMesh");
// 	transformComp->SetPosition(Vector3D(8.0f, -3.0f, -15.0f));
// 	transformComp->SetScale(Vector3D(0.5f, 0.5f, 0.5f));
// 
// 	mEntities.push_back(lampEntity);
// 
// 	//
// 	// 3 - SE
// 	//
// 	lampEntity = RZE_Engine::Get()->GetECS()->AddEntity<GameEntity>();
// 
// 	meshComponent = lampEntity->AddComponent<MeshComponent>("LampMesh");
// 	meshComponent->SetMeshHandle(resourceHandle);
// 	meshComponent->SetShaderGroup(mDefaultShader);
// 
// 	transformComp = lampEntity->AddComponent<TransformComponent>();
// 	transformComp->SetPosition(Vector3D(-8.0f, -3.0f, -2.0f));
// 	transformComp->SetScale(Vector3D(0.5f, 0.5f, 0.5f));
// 
// 	mEntities.push_back(lampEntity);
// 
// 	//
// 	// 4 - SW
// 	//
// 	lampEntity = RZE_Engine::Get()->GetECS()->AddEntity<GameEntity>();
// 
// 	meshComponent = lampEntity->AddComponent<MeshComponent>("LampMesh");
// 	meshComponent->SetMeshHandle(resourceHandle);
// 	meshComponent->SetShaderGroup(mDefaultShader);
// 
// 	transformComp = lampEntity->AddComponent<TransformComponent>();
// 	transformComp->SetPosition(Vector3D(8.0f, -3.0f, -2.0f));
// 	transformComp->SetScale(Vector3D(0.5f, 0.5f, 0.5f));
// 
// 	mEntities.push_back(lampEntity);
// }
// 
// void GameApp::CreateDefaultShader()
// {
// 	const char* const vertShaderFilePath = "./../Engine/Assets/Shaders/VertexShader.shader";
// 	const char* const fragShaderFilePath = "./../Engine/Assets/Shaders/FragmentShader.shader";
// 
// 	ResourceHandle vertShaderHandle = RZE_Engine::Get()->GetResourceHandler().RequestResource<GFXShader>(vertShaderFilePath, EGLShaderType::Vertex, "DefaultVertexShader");
// 	ResourceHandle fragShaderHandle = RZE_Engine::Get()->GetResourceHandler().RequestResource<GFXShader>(fragShaderFilePath, EGLShaderType::Fragment, "DefaultFragShader");
// 
// 	GFXShader* vertShader = RZE_Engine::Get()->GetResourceHandler().GetResource<GFXShader>(vertShaderHandle);
// 	vertShader->Create();
// 	vertShader->Compile();
// 
// 	GFXShader* fragShader = RZE_Engine::Get()->GetResourceHandler().GetResource<GFXShader>(fragShaderHandle);
// 	fragShader->Create();
// 	fragShader->Compile();
// 
// 	mDefaultShader = new GFXShaderGroup("DefaultShader");
// 	mDefaultShader->AddShader(GFXShaderGroup::EShaderIndex::Vertex, vertShader);
// 	mDefaultShader->AddShader(GFXShaderGroup::EShaderIndex::Fragment, fragShader);
// 
// 	mDefaultShader->AddUniform("UModelMat");
// 	mDefaultShader->AddUniform("UProjectionMat");
// 	mDefaultShader->AddUniform("UViewMat");
// 
// 	mDefaultShader->AddUniform("ULightPosition");
// 	mDefaultShader->AddUniform("UViewPosition");
// 	mDefaultShader->AddUniform("ULightColor");
// 	mDefaultShader->AddUniform("ULightStrength");
// 
// 	mDefaultShader->AddUniform("UFragColor");
// 
// 	mDefaultShader->GenerateShaderProgram();
// 
// 	RZE_Engine::Get()->GetResourceHandler().ReleaseResource(vertShaderHandle);
// 	RZE_Engine::Get()->GetResourceHandler().ReleaseResource(fragShaderHandle);
// }
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
