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
			MouseInput(*camComp, *transfComp);

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

void FreeCameraSystem::MouseInput(CameraComponent& camComp, TransformComponent& transfComp)
{
	InputHandler& inputHandler = RZE_Engine::Get()->GetInputHandler();

	if (mMousePrevPos.LengthSq() == 0)
	{
		mMousePrevPos = inputHandler.GetMouseState().CurPosition;
	}

	if (RZE_Engine::Get()->GetInputHandler().GetMouseState().GetButtonState(EMouseButton::MouseButton_Right) == EButtonState::ButtonState_Pressed)
	{
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

// 	if (RZE_Engine::Get()->GetInputHandler().GetMouseState().GetButtonState(EMouseButton::MouseButton_Middle) == EButtonState::ButtonState_Pressed)
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

	if (RZE_Engine::Get()->GetInputHandler().GetMouseState().GetButtonState(EMouseButton::MouseButton_Right) == EButtonState::ButtonState_Released
		&& RZE_Engine::Get()->GetInputHandler().GetMouseState().GetButtonState(EMouseButton::MouseButton_Middle) == EButtonState::ButtonState_Released && mMousePrevPos.LengthSq() > 0)
	{
		mMousePrevPos = Vector3D();
	}
}

void FreeCameraSystem::RegisterComponentAddedNotifications()
{
	Apollo::EntityHandler::ComponentAddedFunc camCompAdded([this](Apollo::EntityID entity, Apollo::EntityHandler& handler)
	{
		//
		// #TODO(Josh) This is broken. This assumes the TransformComponent is created __BEFORE__ the CameraComponent
		// and obviously we cant guarantee this in any way. See GameApp.cpp.
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

Vector3D FreeCameraSystem::ArcBallProjection(const Vector3D& vec)
{
	const Vector2D& winDims = RZE_Engine::Get()->GetWindowSize();
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
