#include <Game/Systems/RotateSystem.h>

#include <RZE.h>

#include <ECS/Components/CameraComponent.h>

#include <Windowing/WinKeyCodes.h>

#include <Utils/Math/Math.h>

const float kSpeed = 5.0f;
const float kWheelSpeed = 10.0f;

RotateSystem::RotateSystem()
{
}

RotateSystem::~RotateSystem()
{
}

void RotateSystem::Initialize()
{
	InternalGetComponentFilter().AddFilterType<CameraComponent>();

	Apollo::ComponentHandler& handler = RZE_Engine::Get()->GetComponentHandler();

	Apollo::ComponentHandler::ComponentAddedFunc camCompAdded([this](Apollo::EntityID entityID, Apollo::ComponentHandler& handler)
	{
		CameraComponent* const camComp = handler.GetComponent<CameraComponent>(entityID);
		AssertNotNull(camComp);
		mMainCamera = camComp;
	});
	handler.RegisterForComponentAddNotification<CameraComponent>(camCompAdded);

	BindInputs();
}

void RotateSystem::Update(std::vector<Apollo::EntityID>& entities)
{
}

void RotateSystem::ShutDown()
{
}

void RotateSystem::BindInputs()
{
	Functor<void, const InputKey&> keyFunc([this](const InputKey& key)
	{
		if (key.GetKeyCode() == Win32KeyCode::Key_W)
		{
			AssertNotNull(mMainCamera);
			mMainCamera->Position += mMainCamera->Forward * kSpeed * RZE_Engine::Get()->GetDeltaTime();
		}
		else if (key.GetKeyCode() == Win32KeyCode::Key_S)
		{
			AssertNotNull(mMainCamera);
			mMainCamera->Position -= mMainCamera->Forward * kSpeed * RZE_Engine::Get()->GetDeltaTime();
		}

		if (key.GetKeyCode() == Win32KeyCode::Key_A)
		{
			AssertNotNull(mMainCamera);
			mMainCamera->Position -= mMainCamera->Forward.Cross(mMainCamera->UpDir).Normalize() * kSpeed * RZE_Engine::Get()->GetDeltaTime();
		}
		else if (key.GetKeyCode() == Win32KeyCode::Key_D)
		{
			AssertNotNull(mMainCamera);
			mMainCamera->Position += mMainCamera->Forward.Cross(mMainCamera->UpDir).Normalize() * kSpeed * RZE_Engine::Get()->GetDeltaTime();
		}

		if (key.GetKeyCode() == Win32KeyCode::Key_Q)
		{
			AssertNotNull(mMainCamera);
			mMainCamera->Position += mMainCamera->UpDir * kSpeed * RZE_Engine::Get()->GetDeltaTime();
		}
		else if (key.GetKeyCode() == Win32KeyCode::Key_E)
		{
			AssertNotNull(mMainCamera);
			mMainCamera->Position -= mMainCamera->UpDir * kSpeed * RZE_Engine::Get()->GetDeltaTime();
		}
	});
	RZE_Engine::Get()->GetInputHandler().BindAction(Win32KeyCode::Key_W, EButtonState::ButtonState_Hold, keyFunc);
	RZE_Engine::Get()->GetInputHandler().BindAction(Win32KeyCode::Key_S, EButtonState::ButtonState_Hold, keyFunc);
	RZE_Engine::Get()->GetInputHandler().BindAction(Win32KeyCode::Key_A, EButtonState::ButtonState_Hold, keyFunc);
	RZE_Engine::Get()->GetInputHandler().BindAction(Win32KeyCode::Key_D, EButtonState::ButtonState_Hold, keyFunc);
	RZE_Engine::Get()->GetInputHandler().BindAction(Win32KeyCode::Key_Q, EButtonState::ButtonState_Hold, keyFunc);
	RZE_Engine::Get()->GetInputHandler().BindAction(Win32KeyCode::Key_E, EButtonState::ButtonState_Hold, keyFunc);

	Functor<void, const Vector3D&, Int32> mouseFunc([this](const Vector3D& axis, Int32 wheel)
	{
		if (wheel > 0)
		{
			mMainCamera->Position += mMainCamera->Forward * kWheelSpeed * RZE_Engine::Get()->GetDeltaTime();
		}
		else if (wheel < 0)
		{
			mMainCamera->Position -= mMainCamera->Forward * kWheelSpeed * RZE_Engine::Get()->GetDeltaTime();
		}
	});
	RZE_Engine::Get()->GetInputHandler().BindAxis(EAxisBinding::AxisBinding_Mouse, EAxisType::AxisType_Vector, mouseFunc);
}
