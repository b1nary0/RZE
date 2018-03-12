#include <Game/Systems/WeirdTempInputSystem.h>

#include <RZE.h>

#include <ECS/Components/CameraComponent.h>

#include <Windowing/WinKeyCodes.h>

#include <Utils/Math/Math.h>
#include <Utils/Math/Matrix4x4.h>



// #TODO(Josh) Test includes for rotation entities
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/TransformComponent.h>



const float kSpeed = 25.0f;
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
		mMainCamera->Forward.Normalize();
		mMainCamera->UpDir.Normalize();
	});
	handler.RegisterForComponentAddNotification<CameraComponent>(camCompAdded);

	CreateEntities();
	BindInputs();
}

void WeirdTempInputSystem::Update(std::vector<Apollo::EntityID>& entities)
{
	Apollo::EntityHandler& handler = RZE_Engine::Get()->GetActiveScene().GetEntityHandler();
	{
		TransformComponent* const transfComp0 = handler.GetComponent<TransformComponent>(mEntity0);
		TransformComponent* const transfComp1 = handler.GetComponent<TransformComponent>(mEntity1);
		
		transfComp0->Position = transfComp1->Position + (Quaternion(Vector3D(0.0f, 0.05f, 0.0f)) * (transfComp0->Position - transfComp1->Position));
	}
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
		if (prevPos.LengthSq() == 0)
		{
			prevPos = axis;
		}

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
			if (RZE_Engine::Get()->GetInputHandler().GetMouseState().GetButtonState(EMouseButton::MouseButton_Right) == EButtonState::ButtonState_Pressed)
			{
				Vector3D diff = axis - prevPos;
				diff = diff * 0.1f; // #TODO(Josh) Move this to a better place (mouse sensitivity) -- config file
				mPitchYawRoll += diff;
				 
				Vector3D lookDir;
				lookDir.SetX(std::cos(mPitchYawRoll.X() * MathUtils::ToRadians) * std::cos(mPitchYawRoll.Y() * MathUtils::ToRadians));
				lookDir.SetY(std::sin(mPitchYawRoll.Y() * MathUtils::ToRadians));
				lookDir.SetZ(std::sin(mPitchYawRoll.X() * MathUtils::ToRadians) * std::cos(mPitchYawRoll.Y() * MathUtils::ToRadians));

				lookDir.SetY(lookDir.Y() * -1);

				mMainCamera->Forward = std::move(lookDir);

				prevPos = axis;

			}
			else if (RZE_Engine::Get()->GetInputHandler().GetMouseState().GetButtonState(EMouseButton::MouseButton_Middle) == EButtonState::ButtonState_Pressed)
			{
				// #TODO(Josh) Messin with some stuff https://en.wikibooks.org/wiki/OpenGL_Programming/Modern_OpenGL_Tutorial_Arcball
				Vector3D curArcBallProj = ArcBallProjection(axis);
				Vector3D prevArcBallProj = ArcBallProjection(prevPos);
				Quaternion rot(curArcBallProj, prevArcBallProj);

				Vector3D orbitPoint = mMainCamera->Position + (mMainCamera->Forward * 5.0f);
				mMainCamera->Position = (orbitPoint + (rot * (mMainCamera->Position - orbitPoint)));
				mMainCamera->Forward = (orbitPoint - mMainCamera->Position).Normalize();

				prevPos = axis;
			}

			if (RZE_Engine::Get()->GetInputHandler().GetMouseState().GetButtonState(EMouseButton::MouseButton_Right) == EButtonState::ButtonState_Released
				&& RZE_Engine::Get()->GetInputHandler().GetMouseState().GetButtonState(EMouseButton::MouseButton_Middle) == EButtonState::ButtonState_Released && prevPos.LengthSq() > 0)
			{
				prevPos = Vector3D();
			}
		}
	});
	RZE_Engine::Get()->GetInputHandler().BindAxis(EAxisBinding::AxisBinding_Mouse, EAxisType::AxisType_Vector, mouseFunc);
}

void WeirdTempInputSystem::CreateEntities()
{
	GameScene& scene = RZE_Engine::Get()->GetActiveScene();
	{
		mEntity0 = scene.GetEntityHandler().CreateEntity();

		scene.GetEntityHandler().AddComponent<MeshComponent>(mEntity0, FilePath("Engine/Assets/3D/Cube.obj"));
		scene.GetEntityHandler().AddComponent<TransformComponent>(mEntity0, Vector3D(0.0f, 2.0f, -6.0f), Quaternion(), Vector3D(1.0f));
	}
	{
		mEntity1 = scene.GetEntityHandler().CreateEntity();

		scene.GetEntityHandler().AddComponent<MeshComponent>(mEntity1, FilePath("Engine/Assets/3D/Cube.obj"));
		scene.GetEntityHandler().AddComponent<TransformComponent>(mEntity1, Vector3D(-4.0f, 2.0f, -6.0f), Quaternion(), Vector3D(1.0f));
	}
}

Vector3D WeirdTempInputSystem::ArcBallProjection(const Vector3D& vec)
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
