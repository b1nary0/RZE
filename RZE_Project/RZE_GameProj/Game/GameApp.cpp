#include "GameApp.h"

#include <RZE_Config.h>
#include <RZE.h>

#include <DebugUtils/Debug.h>

#include <Game/GameEntity.h>
#include <Game/Components/MeshComponent.h>
#include <Game/Components/TransformComponent.h>
#include <Game/Components/LightSourceComponent.h>
#include <Game/Components/FontRenderComponent.h>

#include <RenderCore/Graphics/Mesh.h>
#include <RenderCore/Graphics/Texture2D.h>
#include <RenderCore/Shaders/ShaderGroup.h>

#include <Windowing/WinKeyCodes.h>

GameApp::GameApp()
	: RZE_Game()
{
}

GameApp::~GameApp()
{
}

void GameApp::RegisterEvents(EventHandler& eventHandler)
{
	static float angleX_1 = 0.0f;
	static float angleY_1 = 0.0f;

	static float speed = 25.0f;

	static float deltaT = (1.0f / 60.0f);

	static float pitch = 0;
	static float yaw = 0;

	static Vector3D upVec(0.0f, (speed * deltaT), 0.0f);
	static Vector3D downVec(0.0f, (-speed * deltaT), 0.0f);

	Functor<void, const Event&> keyEvent([this](const Event& evt)
	{
		if (evt.mInfo.mEventType == EEventType::Key)
		{
			if (evt.mKeyEvent.mKey == Win32KeyCode::Key_W)
			{
				SceneCamera& sceneCam = RZE_Engine::Get()->GetSceneCamera();
				Vector3D newPos = sceneCam.GetPositionVec();
				newPos += sceneCam.GetDirectionVec() * speed * deltaT;
				sceneCam.SetPosition(newPos);
			}
			else if (evt.mKeyEvent.mKey == Win32KeyCode::Key_S)
			{
				SceneCamera& sceneCam = RZE_Engine::Get()->GetSceneCamera();
				Vector3D newPos = sceneCam.GetPositionVec();
				newPos -= sceneCam.GetDirectionVec() * speed * deltaT;
				sceneCam.SetPosition(newPos);
			}

			if (evt.mKeyEvent.mKey == Win32KeyCode::Key_A)
			{
				SceneCamera& sceneCam = RZE_Engine::Get()->GetSceneCamera();
				Vector3D newPos = sceneCam.GetPositionVec();
				newPos -= sceneCam.GetDirectionVec().Cross(sceneCam.GetUpVec()).Normalize() * speed * deltaT;
				sceneCam.SetPosition(newPos);
			}
			else if (evt.mKeyEvent.mKey == Win32KeyCode::Key_D)
			{
				SceneCamera& sceneCam = RZE_Engine::Get()->GetSceneCamera();
				Vector3D newPos = sceneCam.GetPositionVec();
				newPos += sceneCam.GetDirectionVec().Cross(sceneCam.GetUpVec()).Normalize() * speed * deltaT;
				sceneCam.SetPosition(newPos);
			}

			if (evt.mKeyEvent.mKey == Win32KeyCode::Key_Q)
			{
				SceneCamera& sceneCam = RZE_Engine::Get()->GetSceneCamera();
				Vector3D newPos = sceneCam.GetPositionVec();
				newPos = newPos + upVec;
				sceneCam.SetPosition(newPos);
			}
			else if (evt.mKeyEvent.mKey == Win32KeyCode::Key_E)
			{
				SceneCamera& sceneCam = RZE_Engine::Get()->GetSceneCamera();
				Vector3D newPos = sceneCam.GetPositionVec();
				newPos = newPos + downVec;
				sceneCam.SetPosition(newPos);
			}
		}
	});
	eventHandler.RegisterForEvent(EEventType::Key, keyEvent);

	Functor<void, const Event&> mouseEvent([this](const Event& event)
	{
		static float pitch = 0;
		static float yaw = 0;

		const float sens = 0.05f;

		if (event.mInfo.mEventSubType == EMouseEventType::Mouse_Move)
		{
			GetWindow()->ResetCursorToCenter();

			float posX = static_cast<float>(event.mMouseEvent.mPosX);
			float posY = static_cast<float>(event.mMouseEvent.mPosY);
			float halfWidth = GetWindow()->GetDimensions().X() / 2;
			float halfHeight = GetWindow()->GetDimensions().Y() / 2;

			float xPosOffset = posX - halfWidth;
			float yPosOffset = halfHeight - posY;

			xPosOffset *= sens;
			yPosOffset *= sens;

			yaw += xPosOffset;
			pitch += yPosOffset;

			// #TODO(Josh) move glm::radians into somewhere more better.
			float newDirX = std::cos(glm::radians(pitch)) * std::cos(glm::radians(yaw));
			float newDirY = std::sin(glm::radians(pitch));
			float newDirZ = std::cos(glm::radians(pitch)) * std::sin(glm::radians(yaw));

			Vector3D newDir(newDirX, newDirY, newDirZ);
			newDir.Normalize();
			RZE_Engine::Get()->GetSceneCamera().SetDirection(newDir);
		}
	});
	eventHandler.RegisterForEvent(EEventType::Mouse, mouseEvent);
}

void GameApp::Start()
{
	RZE_Game::Start();

	// ALL TEST CODE

	CreateDefaultShader();
	CreateTextureShader();

	const char* const cubeFilePath = "./../RZE_Engine/Assets/3D/Cube.obj";
	const char* const miniCooperFilePath = "./../RZE_Engine/Assets/3D/MiniCooper.obj";
	const char* const lampFilePath = "./../RZE_Engine/Assets/3D/Lamp.obj";

	const char* const quadMeshFilePath = "./../RZE_Engine/Assets/3D/Quad.obj";
	const char* const quadTextureFilePath = "./../RZE_Engine/Assets/2D/Container.jpg";

	ResourceHandler& resourceHandler = RZE_Engine::Get()->GetResourceHandler();

	ResourceHandle cubeMesh = resourceHandler.RequestResource<MeshResource>(cubeFilePath);
	ResourceHandle lampMesh = resourceHandler.RequestResource<MeshResource>(lampFilePath);

	ResourceHandle quadMesh = resourceHandler.RequestResource<MeshResource>(quadMeshFilePath);
	ResourceHandle quadTex = resourceHandler.RequestResource<GFXTexture2D>(quadTextureFilePath);

	CreateLight(cubeMesh);
	CreateGround(cubeMesh);
	CreateLampObjects(lampMesh);
	CreateTextureQuad(quadMesh, quadTex);
}

void GameApp::Update()
{
	RZE_Game::Update();
}

void GameApp::CreateTextureQuad(const ResourceHandle& meshHandle, const ResourceHandle& textureHandle)
{
	GameEntity* entity = RZE_Engine::Get()->GetWorld()->AddEntity<GameEntity>("TextureQuad");

	MeshComponent* const meshComp = entity->AddComponent<MeshComponent>();
	meshComp->SetMeshHandle(meshHandle);
	meshComp->SetTextureHandle(textureHandle);
	meshComp->SetShaderGroup(mTextureShader);

	TransformComponent* const transfComp = entity->AddComponent<TransformComponent>();
	transfComp->SetPosition(Vector3D(0.0f, 2.0f, -3.0f));
	transfComp->SetScale(Vector3D(4.0f, 3.0f, 0.0f));

	mEntities.push_back(entity);
}

void GameApp::CreateFontTest()
{
	GameEntity* entity = RZE_Engine::Get()->GetWorld()->AddEntity<GameEntity>();

	FontRenderComponent* const fontComp = entity->AddComponent<FontRenderComponent>("TestFontComponent");
	fontComp->SetFont(RZE_Engine::Get()->GetFontHandler().GetFont("Arial"));
	fontComp->SetText("Hello, world!");

	mEntities.push_back(entity);
}

void GameApp::CreateLight(const ResourceHandle& resourceHandle)
{
	mLightEntity = RZE_Engine::Get()->GetWorld()->AddEntity<GameEntity>();

	// 	MeshComponent* lightMesh = mLightEntity->AddComponent<MeshComponent>();
	// 	lightMesh->SetMeshHandle(resourceHandle);
	// 	lightMesh->SetShaderGroup(mDefaultShader);

	TransformComponent* lightTransform = mLightEntity->AddComponent<TransformComponent>();
	lightTransform->SetPosition(Vector3D(-8.0f, 15.0f, -2.0f));

	LightSourceComponent* lightComponent = mLightEntity->AddComponent<LightSourceComponent>("MainTestLight");
	lightComponent->SetColor(Vector3D(1.0f, 1.0f, 1.0f));
	lightComponent->SetStrength(0.75f);

	mEntities.push_back(mLightEntity);
}

void GameApp::CreateGround(const ResourceHandle& resourceHandle)
{
	GameEntity* groundEntity = RZE_Engine::Get()->GetWorld()->AddEntity<GameEntity>();

	MeshComponent* const meshComponent = groundEntity->AddComponent<MeshComponent>();
	meshComponent->SetMeshHandle(resourceHandle);
	meshComponent->SetShaderGroup(mDefaultShader);

	TransformComponent* const transformComp = groundEntity->AddComponent<TransformComponent>();
	transformComp->SetPosition(Vector3D(-10.0f, -5.0f, -20.0f));
	transformComp->SetRotation(Quaternion(Vector3D(0.0f, 0.0f, 0.0f)));
	transformComp->SetScale(Vector3D(20.0f, 0.5f, 20.0f));

	mEntities.push_back(groundEntity);
}

void GameApp::CreateLampObjects(const ResourceHandle& resourceHandle)
{

	//
	// 1 - NE
	//
	GameEntity* lampEntity = RZE_Engine::Get()->GetWorld()->AddEntity<GameEntity>();

	MeshComponent* meshComponent = lampEntity->AddComponent<MeshComponent>("LampMesh");
	meshComponent->SetMeshHandle(resourceHandle);
	meshComponent->SetShaderGroup(mDefaultShader);

	TransformComponent* transformComp = lampEntity->AddComponent<TransformComponent>();
	transformComp->SetPosition(Vector3D(-8.0f, -3.0f, -15.0f));
	transformComp->SetScale(Vector3D(0.5f, 0.5f, 0.5f));

	mEntities.push_back(lampEntity);

	//
	// 2 - NW
	//
	lampEntity = RZE_Engine::Get()->GetWorld()->AddEntity<GameEntity>();

	meshComponent = lampEntity->AddComponent<MeshComponent>("LampMesh");
	meshComponent->SetMeshHandle(resourceHandle);
	meshComponent->SetShaderGroup(mDefaultShader);

	transformComp = lampEntity->AddComponent<TransformComponent>("LampMesh");
	transformComp->SetPosition(Vector3D(8.0f, -3.0f, -15.0f));
	transformComp->SetScale(Vector3D(0.5f, 0.5f, 0.5f));

	mEntities.push_back(lampEntity);

	//
	// 3 - SE
	//
	lampEntity = RZE_Engine::Get()->GetWorld()->AddEntity<GameEntity>();

	meshComponent = lampEntity->AddComponent<MeshComponent>("LampMesh");
	meshComponent->SetMeshHandle(resourceHandle);
	meshComponent->SetShaderGroup(mDefaultShader);

	lampEntity->AddComponent<TransformComponent>();
	transformComp = static_cast<TransformComponent* const>(lampEntity->GetComponents()[1]);
	transformComp->SetPosition(Vector3D(-8.0f, -3.0f, -2.0f));
	transformComp->SetScale(Vector3D(0.5f, 0.5f, 0.5f));

	mEntities.push_back(lampEntity);

	//
	// 4 - SW
	//
	lampEntity = RZE_Engine::Get()->GetWorld()->AddEntity<GameEntity>();

	meshComponent = lampEntity->AddComponent<MeshComponent>("LampMesh");
	meshComponent->SetMeshHandle(resourceHandle);
	meshComponent->SetShaderGroup(mDefaultShader);

	transformComp = lampEntity->AddComponent<TransformComponent>();
	transformComp->SetPosition(Vector3D(8.0f, -3.0f, -2.0f));
	transformComp->SetScale(Vector3D(0.5f, 0.5f, 0.5f));

	mEntities.push_back(lampEntity);
}

void GameApp::CreateDefaultShader()
{
	const char* const vertShaderFilePath = "./../RZE_Engine/Assets/Shaders/VertexShader.shader";
	const char* const fragShaderFilePath = "./../RZE_Engine/Assets/Shaders/FragmentShader.shader";

	ResourceHandle vertShaderHandle = RZE_Engine::Get()->GetResourceHandler().RequestResource<GFXShader>(vertShaderFilePath, EGLShaderType::Vertex, "DefaultVertexShader");
	ResourceHandle fragShaderHandle = RZE_Engine::Get()->GetResourceHandler().RequestResource<GFXShader>(fragShaderFilePath, EGLShaderType::Fragment, "DefaultFragShader");

	GFXShader* vertShader = RZE_Engine::Get()->GetResourceHandler().GetResource<GFXShader>(vertShaderHandle);
	vertShader->Create();
	vertShader->Compile();

	GFXShader* fragShader = RZE_Engine::Get()->GetResourceHandler().GetResource<GFXShader>(fragShaderHandle);
	fragShader->Create();
	fragShader->Compile();

	mDefaultShader = new GFXShaderGroup("DefaultShader");
	mDefaultShader->AddShader(GFXShaderGroup::EShaderIndex::Vertex, vertShader);
	mDefaultShader->AddShader(GFXShaderGroup::EShaderIndex::Fragment, fragShader);

	mDefaultShader->AddUniform("UModelMat");
	mDefaultShader->AddUniform("UProjectionMat");
	mDefaultShader->AddUniform("UViewMat");

	mDefaultShader->AddUniform("ULightPosition");
	mDefaultShader->AddUniform("UViewPosition");
	mDefaultShader->AddUniform("ULightColor");
	mDefaultShader->AddUniform("ULightStrength");

	mDefaultShader->AddUniform("UFragColor");

	mDefaultShader->GenerateShaderProgram();

	RZE_Engine::Get()->GetResourceHandler().ReleaseResource(vertShaderHandle);
	RZE_Engine::Get()->GetResourceHandler().ReleaseResource(fragShaderHandle);
}

void GameApp::CreateTextureShader()
{
	const char* const vertShaderFilePath = "./../RZE_Engine/Assets/Shaders/TextureVert.shader";
	const char* const fragShaderFilePath = "./../RZE_Engine/Assets/Shaders/TextureFrag.shader";

	ResourceHandle vertShaderHandle = RZE_Engine::Get()->GetResourceHandler().RequestResource<GFXShader>(vertShaderFilePath, EGLShaderType::Vertex, "TextureVertShader");
	ResourceHandle fragShaderHandle = RZE_Engine::Get()->GetResourceHandler().RequestResource<GFXShader>(fragShaderFilePath, EGLShaderType::Fragment, "TextureFragShader");

	GFXShader* vertShader = RZE_Engine::Get()->GetResourceHandler().GetResource<GFXShader>(vertShaderHandle);
	vertShader->Create();
	vertShader->Compile();

	GFXShader* fragShader = RZE_Engine::Get()->GetResourceHandler().GetResource<GFXShader>(fragShaderHandle);
	fragShader->Create();
	fragShader->Compile();

	mTextureShader = new GFXShaderGroup("TextureShader");
	mTextureShader->AddShader(GFXShaderGroup::EShaderIndex::Vertex, vertShader);
	mTextureShader->AddShader(GFXShaderGroup::EShaderIndex::Fragment, fragShader);

	mTextureShader->AddUniform("UModelMat");
	mTextureShader->AddUniform("UProjectionMat");
	mTextureShader->AddUniform("UViewMat");

	mTextureShader->AddUniform("ULightPosition");
	mTextureShader->AddUniform("UViewPosition");
	mTextureShader->AddUniform("ULightColor");
	mTextureShader->AddUniform("ULightStrength");

	mTextureShader->AddUniform("UFragColor");
	//mTextureShader->AddUniform("UTexture2D");

	mTextureShader->GenerateShaderProgram();

	RZE_Engine::Get()->GetResourceHandler().ReleaseResource(vertShaderHandle);
	RZE_Engine::Get()->GetResourceHandler().ReleaseResource(fragShaderHandle);
}
