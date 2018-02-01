#include <Game/Systems/WeirdTempInputSystem.h>

#include <RZE.h>

#include <ECS/Components/CameraComponent.h>

#include <Windowing/WinKeyCodes.h>

#include <Utils/Math/Math.h>
#include <Utils/Math/Matrix4x4.h>

const float kSpeed = 5.0f;
const float kWheelSpeed = 10.0f;

WeirdTempInputSystem::WeirdTempInputSystem()
	: bIsMouseDown(false)
{
}

WeirdTempInputSystem::~WeirdTempInputSystem()
{
}

void WeirdTempInputSystem::Initialize()
{
	InternalGetComponentFilter().AddFilterType<CameraComponent>();

	Apollo::EntityHandler& handler = RZE_Engine::Get()->GetActiveScene().GetEntityHandler();

	Apollo::EntityHandler::ComponentAddedFunc camCompAdded([this](Apollo::EntityID entityID, Apollo::EntityHandler& handler)
	{
		CameraComponent* const camComp = handler.GetComponent<CameraComponent>(entityID);
		AssertNotNull(camComp);
		mMainCamera = camComp;
	});
	handler.RegisterForComponentAddNotification<CameraComponent>(camCompAdded);

	BindInputs();
}

void WeirdTempInputSystem::Update(std::vector<Apollo::EntityID>& entities)
{
}

void WeirdTempInputSystem::ShutDown()
{
}

void WeirdTempInputSystem::BindInputs()
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

	static Vector3D prevPos;
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
		else
		{
			if (RZE_Engine::Get()->GetInputHandler().GetMouseState().GetButtonState(EMouseButton::MouseButton_Left) == EButtonState::ButtonState_Pressed)
			{
				if (prevPos.LengthSq() == 0)
				{
					prevPos = axis;
				}

				if (RZE_Engine::Get()->GetInputHandler().GetKeyboardState().GetButtonState(Win32KeyCode::Key_F) == EButtonState::ButtonState_Hold)
				{
					Vector3D camPos = mMainCamera->Position;

					Matrix4x4 translate;
					translate.Translate(Vector3D());
				}
				else
				{
					Vector3D diff = axis - prevPos;
					diff.SetY(diff.Y() * -1);
					mMainCamera->Position -= (diff * RZE_Engine::Get()->GetDeltaTime());

					prevPos = axis;
				}
			}
			else if (RZE_Engine::Get()->GetInputHandler().GetMouseState().GetButtonState(EMouseButton::MouseButton_Left) == EButtonState::ButtonState_Released)
			{
				prevPos = Vector3D();
			}
		}
	});
	RZE_Engine::Get()->GetInputHandler().BindAxis(EAxisBinding::AxisBinding_Mouse, EAxisType::AxisType_Vector, mouseFunc);
}
