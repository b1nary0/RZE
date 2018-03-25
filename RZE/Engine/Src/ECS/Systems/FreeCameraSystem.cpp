#include <StdAfx.h>

#include <ECS/Systems/FreeCameraSystem.h>

#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/TransformComponent.h>

#include <Utils/Math/Math.h>

FreeCameraSystem::FreeCameraSystem()
{
}

FreeCameraSystem::~FreeCameraSystem()
{
}

void FreeCameraSystem::Initialize()
{
	InternalGetComponentFilter().AddFilterType<CameraComponent>();
	InternalGetComponentFilter().AddFilterType<TransformComponent>();

	RegisterComponentAddedNotifications();
}

void FreeCameraSystem::Update(std::vector<Apollo::EntityID>& entities)
{
	Apollo::EntityHandler& handler = RZE_Engine::Get()->GetActiveScene().GetEntityHandler();
	for (auto& entity : entities)
	{
		CameraComponent* const camComp = handler.GetComponent<CameraComponent>(entity);
		TransformComponent* const transfComp = handler.GetComponent<TransformComponent>(entity);
		AssertNotNull(camComp);
		AssertNotNull(transfComp);

		if (camComp->bIsActiveCamera)
		{
			KeyboardInput(*camComp, *transfComp);

			Vector3D dist = mMoveToPoint - transfComp->Position;
			if (dist.LengthSq() > VectorUtils::kEpsilon * VectorUtils::kEpsilon)
			{
				transfComp->Position = VectorUtils::Lerp(transfComp->Position, mMoveToPoint, 0.05f);
			}
		}
	}
}

void FreeCameraSystem::ShutDown()
{
}

void FreeCameraSystem::KeyboardInput(CameraComponent& camComp, TransformComponent& transfComp)
{
	InputHandler& inputHandler = RZE_Engine::Get()->GetInputHandler();

	float dt = static_cast<float>(RZE_Engine::Get()->GetDeltaTime());

	if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_W])
	{
		mMoveToPoint = transfComp.Position + camComp.Forward * mSpeed;
	}
	else if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_S])
	{
		mMoveToPoint = transfComp.Position - camComp.Forward * mSpeed;
	}

	if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_A])
	{
		mMoveToPoint = transfComp.Position - camComp.Forward.Cross(camComp.UpDir).Normalize() * mSpeed;
	}
	else if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_D])
	{
		mMoveToPoint = transfComp.Position + camComp.Forward.Cross(camComp.UpDir).Normalize() * mSpeed;
	}

	if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_Q])
	{
		mMoveToPoint = transfComp.Position + camComp.UpDir * mSpeed;
	}
	else if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_E])
	{
		mMoveToPoint = transfComp.Position - camComp.UpDir * mSpeed;
	}
}

void FreeCameraSystem::RegisterComponentAddedNotifications()
{
	Apollo::EntityHandler::ComponentAddedFunc camCompAdded([this](Apollo::EntityID entity, Apollo::EntityHandler& handler)
	{
		//
		// #TODO(Josh) This is broken. This assumes the TransformComponent is created __BEFORE__ the CameraComponent
		// and obviously we cant guarantee this in any way.
		// This is just written for now for stuff to work but when this system matures this should be fixed or reworked.
		//

		CameraComponent* const camComp = handler.GetComponent<CameraComponent>(entity);
		TransformComponent* const transfComp = handler.GetComponent<TransformComponent>(entity);
		AssertNotNull(transfComp);

		if (camComp->bIsActiveCamera)
		{
			mMoveToPoint = transfComp->Position;
		}
	});
	RZE_Engine::Get()->GetActiveScene().GetEntityHandler().RegisterForComponentAddNotification<CameraComponent>(camCompAdded);
}
