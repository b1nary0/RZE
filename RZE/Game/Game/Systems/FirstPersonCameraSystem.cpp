#include <Game/Systems/FirstPersonCameraSystem.h>

#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/NameComponent.h>
#include <ECS/Components/TransformComponent.h>

#include <Utils/Math/Math.h>

#include <Game/GameApp.h>

#include <Optick/optick.h>

FirstPersonCameraSystem::FirstPersonCameraSystem(Apollo::EntityHandler* const entityHandler)
	: Apollo::EntitySystem(entityHandler)
{
}

FirstPersonCameraSystem::~FirstPersonCameraSystem()
{
}

void FirstPersonCameraSystem::Initialize()
{
	RegisterForComponentNotifications();

	InternalGetComponentFilter().AddFilterType<CameraComponent>();
}

void FirstPersonCameraSystem::Update(const std::vector<Apollo::EntityID>& entities)
{
	OPTICK_EVENT();

	for (Apollo::EntityID entity : entities)
	{
		CameraComponent* const camera = InternalGetEntityHandler().GetComponent<CameraComponent>(entity);
		TransformComponent* const transform = InternalGetEntityHandler().GetComponent<TransformComponent>(entity);

		DoInput(*camera, *transform);

		Vector3D dist = mMoveToPoint - transform->Position;
		if (dist.LengthSq() > VectorUtils::kEpsilon * VectorUtils::kEpsilon)
		{
			Vector3D lerpPos = VectorUtils::Lerp(transform->Position, mMoveToPoint, static_cast<float>(16 * RZE_Application::RZE().GetDeltaTime()));
			transform->SetPosition(lerpPos);
		}
	}
}

void FirstPersonCameraSystem::ShutDown()
{

}

void FirstPersonCameraSystem::RegisterForComponentNotifications()
{
	Apollo::EntityHandler& handler = InternalGetEntityHandler();

	Apollo::EntityHandler::ComponentAddedFunc transfCompAdded([this, &handler](Apollo::EntityID entity)
	{
		NameComponent* const nameComp = handler.GetComponent<NameComponent>(entity);
		TransformComponent* const transfComp = handler.GetComponent<TransformComponent>(entity);
		AssertNotNull(transfComp);
		AssertNotNull(nameComp);

		if (nameComp->Name == "Camera")
		{
			mCamera = entity;
			mMoveToPoint = transfComp->Position;
		}
	});
	handler.RegisterForComponentAddNotification<TransformComponent>(transfCompAdded);
}

void FirstPersonCameraSystem::DoInput(CameraComponent& camera, TransformComponent& transform)
{
	const InputHandler& inputHandler = RZE_Application::RZE().GetInputHandler();

	if (!inputHandler.GetKeyboardState().IsIdle())
	{
		Vector3D newForward;
		Vector3D newStrafe;

		// #TODO(Josh::Need to pay some attention to improving input API to allow for a better non-poll solution)
		if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_W])
		{
			newForward += camera.Forward * mSpeed;
		}
		else if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_S])
		{
			newForward -= camera.Forward * mSpeed;
		}

		if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_A])
		{
			newStrafe -= camera.Forward.Cross(camera.UpDir).Normalize() / 2.0f * mSpeed;
		}
		else if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_D])
		{
			newStrafe += camera.Forward.Cross(camera.UpDir).Normalize() / 2.0f * mSpeed;
		}

		mMoveToPoint = transform.Position + (newForward + newStrafe);
	}

	{
		// Mouse input 
		const Vector3D& curPos = inputHandler.GetMouseState().CurPosition;

		CameraComponent& camComp = *InternalGetEntityHandler().GetComponent<CameraComponent>(mCamera);
		const TransformComponent& transfComp = *InternalGetEntityHandler().GetComponent<TransformComponent>(mCamera);

		Vector3D diff = curPos - mMousePrevPos;

		const float sensitivity = 0.1f;
		diff *= sensitivity;
		mYawPitchRoll += diff;

		float yawInRadians = mYawPitchRoll.X() * MathUtils::ToRadians;
		float pitchInRadians = mYawPitchRoll.Y() * MathUtils::ToRadians;

		Vector3D newForward;
		newForward.SetX(std::cos(yawInRadians) * std::cos(pitchInRadians));
		newForward.SetY(-std::sin(pitchInRadians));
		newForward.SetZ(std::sin(yawInRadians) * std::cos(pitchInRadians));

		camComp.Forward = newForward;
		camComp.Forward.Normalize();

		mMousePrevPos = curPos;
	}
}
