#include <ECS/Systems/FreeCameraSystem.h>

#include <algorithm>

#include <Brofiler/Brofiler.h>

#include <EngineApp.h>

#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/NameComponent.h>
#include <ECS/Components/TransformComponent.h>

#include <Utils/Math/Math.h>

FreeCameraSystem::FreeCameraSystem(Apollo::EntityHandler* const entityHandler)
	: Apollo::EntitySystem(entityHandler)
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

void FreeCameraSystem::Update(const std::vector<Apollo::EntityID>& entities)
{
	BROFILER_CATEGORY("FreeCameraSystem::Update", Profiler::Color::Yellow);

	Apollo::EntityHandler& handler = InternalGetEntityHandler();
	for (auto& entity : entities)
	{
		CameraComponent* const camComp = handler.GetComponent<CameraComponent>(entity);
		AssertNotNull(camComp);

		if (camComp->bIsActiveCamera)
		{
			TransformComponent* const transfComp = handler.GetComponent<TransformComponent>(entity);
			AssertNotNull(transfComp);

			KeyboardInput(*camComp, *transfComp);
			MouseInput(*camComp, *transfComp);
		}
	}

}

void FreeCameraSystem::ShutDown()
{
}

void FreeCameraSystem::KeyboardInput(CameraComponent& camComp, TransformComponent& transfComp)
{
	InputHandler& inputHandler = RZE_Application::RZE().GetInputHandler();

	float dt = static_cast<float>(RZE_Application::RZE().GetDeltaTime());
	float speedDelta = mSpeed * dt;
	if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_W])
	{
		transfComp.Position += camComp.Forward * speedDelta;
	}
	else if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_S])
	{
		transfComp.Position -= camComp.Forward * speedDelta;
	}
	else if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_A])
	{
		transfComp.Position -= camComp.Forward.Cross(camComp.UpDir).Normalize() * speedDelta;
	}
	else if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_D])
	{
		transfComp.Position += camComp.Forward.Cross(camComp.UpDir).Normalize() * speedDelta;
	}
	else if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_E])
	{
		transfComp.Position += camComp.Forward.Cross(camComp.UpDir).Cross(camComp.Forward) * speedDelta;
	}
	else if (inputHandler.GetKeyboardState().CurKeyStates[Win32KeyCode::Key_Q])
	{
		transfComp.Position -= camComp.Forward.Cross(camComp.UpDir).Cross(camComp.Forward) * speedDelta;
	}

	Int32 wheelVal = RZE_Application::RZE().GetInputHandler().GetMouseState().CurWheelVal;
	if (wheelVal != 0)
	{
		wheelVal = MathUtils::Clamp(wheelVal, -1, 1);
		transfComp.Position = transfComp.Position + (camComp.Forward * static_cast<float>(wheelVal)) * mWheelZoomSpeed;
	}
}

void FreeCameraSystem::MouseInput(CameraComponent& camComp, TransformComponent& transfComp)
{
	InputHandler& inputHandler = RZE_Application::RZE().GetInputHandler();

	if (RZE_Application::RZE().GetInputHandler().GetMouseState().GetButtonState(EMouseButton::MouseButton_Right) == EButtonState::ButtonState_Pressed)
	{
		if (mMousePrevPos.LengthSq() == 0)
		{
			mMousePrevPos = inputHandler.GetMouseState().CurPosition;
		}

		Vector3D curPos = inputHandler.GetMouseState().CurPosition;
		Vector3D diff = curPos - mMousePrevPos;
		diff = diff * 0.1f; // #TODO(Josh) Move this to a better place (mouse sensitivity) -- config file
		mPitchYawRoll += diff;

		Vector3D lookDir;
		lookDir.SetX(std::cos(mPitchYawRoll.X() * MathUtils::ToRadians) * std::cos(mPitchYawRoll.Y() * MathUtils::ToRadians));
		lookDir.SetY(std::sin(mPitchYawRoll.Y() * MathUtils::ToRadians));
		lookDir.SetZ(std::sin(mPitchYawRoll.X() * MathUtils::ToRadians) * std::cos(mPitchYawRoll.Y() * MathUtils::ToRadians));

		lookDir.SetY(lookDir.Y() * -1);

		camComp.Forward = std::move(lookDir);

		mMousePrevPos = curPos;
	}
// 	if (RZE_Application::RZE().GetInputHandler().GetMouseState().GetButtonState(EMouseButton::MouseButton_Middle) == EButtonState::ButtonState_Pressed)
// 	{
// 		Vector3D curPos = inputHandler.GetMouseState().CurPosition;
// 
// 		// #TODO(Josh) Messin with some stuff https://en.wikibooks.org/wiki/OpenGL_Programming/Modern_OpenGL_Tutorial_Arcball
// 		Vector3D curArcBallProj = ArcBallProjection(curPos);
// 		Vector3D prevArcBallProj = ArcBallProjection(mMousePrevPos);
// 		Quaternion rot(curArcBallProj, prevArcBallProj);
// 
// 		if (mOrbitPoint.LengthSq() == 0.0f)
// 		{
// 			mOrbitPoint = transfComp.Position + (camComp.Forward * 5.0f);
// 		}
// 
// 		mMoveToPoint =  (mOrbitPoint + (rot * (transfComp.Position - mOrbitPoint)));
// 		camComp.Forward = (mOrbitPoint - transfComp.Position).Normalize();
// 
// 		mMousePrevPos = curPos;
// 	}

	if (RZE_Application::RZE().GetInputHandler().GetMouseState().GetButtonState(EMouseButton::MouseButton_Right) == EButtonState::ButtonState_Released
		&& RZE_Application::RZE().GetInputHandler().GetMouseState().GetButtonState(EMouseButton::MouseButton_Middle) == EButtonState::ButtonState_Released && mMousePrevPos.LengthSq() > 0)
	{
		mMousePrevPos = Vector3D();
	}
}

void FreeCameraSystem::RegisterComponentAddedNotifications()
{
}

Vector3D FreeCameraSystem::ArcBallProjection(const Vector3D& vec)
{
	const Vector2D& winDims = RZE_Application::RZE().GetWindowSize();
	float x = 1.0f * vec.X() / winDims.X() * 2.0f - 1.0f;
	float y = 1.0f * vec.Y() / winDims.Y() * 2.0f - 1.0f;

	y = -y;

	float sq = x * x + y * y;
	if (sq <= 1)
	{
		float z = sqrt(1 - sq);
		return Vector3D(x, y, z);
	}

	return Vector3D(x, y, 0.0f).Normalize();
}
