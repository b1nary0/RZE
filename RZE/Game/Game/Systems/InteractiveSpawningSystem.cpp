#include <StdAfx.h>
#include <Game/Systems/InteractiveSpawningSystem.h>

#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/TransformComponent.h>

#include <random>

static const char* assetPaths[2] =
{
	"Engine/Assets/3D/FW190/FW190.obj",
	"Engine/Assets/3D/Panzer/Panzer.obj"
};

static int sPossibleValues[8] =
{
	5, 2, 0, 0, 6, -2, -4, -8
};

void InteractiveSpawningSystem::Initialize()
{
	srand(static_cast<unsigned int>(time(NULL)));
}

void InteractiveSpawningSystem::Update(const std::vector<Apollo::EntityID>& entities)
{
	HackyInputThing();
}

void InteractiveSpawningSystem::ShutDown()
{

}

void InteractiveSpawningSystem::HackyInputThing()
{
	static bool bSwapAsset = false;

	InputHandler& inputHandler = RZE_Application::RZE().GetInputHandler();

	if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_A] == EButtonState::ButtonState_Pressed
		&& inputHandler.GetKeyboardState().PrevKeyStates[Win32KeyCode::Key_A] == EButtonState::ButtonState_Released)
	{
		Apollo::EntityID entity = RZE_Application::RZE().GetActiveScene().CreateEntity("SpawnedObject");
		InternalGetEntityHandler().AddComponent<MeshComponent>(entity, FilePath(assetPaths[bSwapAsset]));

		TransformComponent* const transfComp = InternalGetEntityHandler().GetComponent<TransformComponent>(entity);
		transfComp->SetPosition(Vector3D(rand() % 8, rand() % 8, rand() % 8));

		bSwapAsset = !bSwapAsset;
	}
}
