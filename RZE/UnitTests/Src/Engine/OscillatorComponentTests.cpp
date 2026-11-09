#include <Utils/DebugUtils/DebugConfig.h>

// The component is development-only, so are its tests
#ifdef RZE_DEVEL

#include <doctest/doctest.h>

#include <RZE.h>

#include <Game/GameScene.h>
#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/OscillatorComponent.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <RapidJSON/document.h>

namespace
{
	void CheckPosition(const Vector3D& position, const Vector3D& expected)
	{
		for (int axis = 0; axis < 3; ++axis)
		{
			CHECK(position[axis] == doctest::Approx(expected[axis]).epsilon(0.0001));
		}
	}

	// An object at Start with an oscillator: 3 m either side along +Y, 4 s a cycle
	struct Oscillating
	{
		const Vector3D Start = Vector3D(5.0f, 1.0f, -2.0f);

		GameScene Scene;
		GameObjectPtr Object;
		GameObjectComponentPtr<OscillatorComponent> Oscillator;

		Oscillating()
		{
			Object = Scene.AddGameObject("Oscillating");
			Object->GetTransformComponent()->SetPosition(Start);
			Oscillator = Object->AddComponent<OscillatorComponent>();
			// Any length: only the direction counts
			Oscillator->SetAxis(Vector3D(0.0f, 2.0f, 0.0f));
			Oscillator->SetAmplitude(3.0f);
			Oscillator->SetPeriod(4.0f);
		}
		~Oscillating() { Scene.Unload(); }

		const Vector3D& Position() { return Object->GetTransformComponent()->GetPosition(); }
	};
}

TEST_CASE_FIXTURE(Oscillating, "An oscillator added to an object moves around where the object was")
{
	CheckPosition(Oscillator->GetOrigin(), Start);

	Oscillator->Advance(0.0);
	CheckPosition(Position(), Start);
	Oscillator->Advance(1.0);
	CheckPosition(Position(), Start + Vector3D(0.0f, 3.0f, 0.0f));
	Oscillator->Advance(1.0);
	CheckPosition(Position(), Start);
	Oscillator->Advance(1.0);
	CheckPosition(Position(), Start - Vector3D(0.0f, 3.0f, 0.0f));
	Oscillator->Advance(1.0);
	CheckPosition(Position(), Start);
}

TEST_CASE_FIXTURE(Oscillating, "The phase starts the motion part of a cycle in")
{
	Oscillator->SetPhase(0.25f);
	Oscillator->Advance(0.0);
	CheckPosition(Position(), Start + Vector3D(0.0f, 3.0f, 0.0f));
	Oscillator->Advance(2.0);
	CheckPosition(Position(), Start - Vector3D(0.0f, 3.0f, 0.0f));
}

TEST_CASE_FIXTURE(Oscillating, "However many small steps it takes, it's where the time says, with no drift")
{
	// 10,000 cycles in steps that don't divide a period evenly in binary, like frame times
	const double step = 4.0 / 7.0;
	for (int i = 0; i < 70000; ++i)
	{
		Oscillator->Advance(step);
	}
	CheckPosition(Position(), Start);

	// And a quarter cycle on from there
	Oscillator->Advance(1.0);
	CheckPosition(Position(), Start + Vector3D(0.0f, 3.0f, 0.0f));
}

TEST_CASE_FIXTURE(Oscillating, "Turning it off puts the object back; turning it on makes where the object is the Origin")
{
	Oscillator->Advance(1.0);
	Oscillator->SetEnabled(false);
	CheckPosition(Position(), Start);

	// Off, it leaves the object alone, wherever it's put
	const Vector3D moved(20.0f, 0.0f, 0.0f);
	Object->GetTransformComponent()->SetPosition(moved);
	Oscillator->Advance(1.0);
	CheckPosition(Position(), moved);

	Oscillator->SetEnabled(true);
	CheckPosition(Oscillator->GetOrigin(), moved);
	Oscillator->Advance(2.0);
	CheckPosition(Position(), moved - Vector3D(0.0f, 3.0f, 0.0f));
}

TEST_CASE_FIXTURE(Oscillating, "While it's on it owns the position: moving it means moving its Origin")
{
	Object->GetTransformComponent()->SetPosition(Vector3D(100.0f, 0.0f, 0.0f));
	Oscillator->Advance(0.0);
	CheckPosition(Position(), Start);

	const Vector3D origin(-4.0f, 0.0f, 8.0f);
	Oscillator->SetOrigin(origin);
	Oscillator->Advance(1.0);
	CheckPosition(Position(), origin + Vector3D(0.0f, 3.0f, 0.0f));
}

TEST_CASE_FIXTURE(Oscillating, "With no period or no axis it stays at its Origin")
{
	Oscillator->SetPeriod(0.0f);
	Oscillator->Advance(1.0);
	CheckPosition(Position(), Start);

	Oscillator->SetPeriod(4.0f);
	Oscillator->SetAxis(Vector3D());
	Oscillator->Advance(1.0);
	CheckPosition(Position(), Start);
}

TEST_CASE_FIXTURE(Oscillating, "A saved oscillator loads back the same, Origin included")
{
	Oscillator->SetPhase(0.5f);
	Oscillator->SetOrigin(Vector3D(1.0f, 2.0f, 3.0f));
	Oscillator->SetEnabled(false);

	rapidjson::StringBuffer buffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
	writer.StartObject();
	Oscillator->Serialize(writer);
	writer.EndObject();

	rapidjson::Document document;
	document.Parse(buffer.GetString());
	REQUIRE(document.HasMember("OscillatorComponent"));

	// Loaded onto an object somewhere else, which Initialize would take as the Origin if the file hadn't one
	GameObjectPtr loaded = Scene.AddGameObject("Loaded");
	loaded->GetTransformComponent()->SetPosition(Vector3D(50.0f, 0.0f, 0.0f));
	GameObjectComponentPtr<OscillatorComponent> copy = loaded->AddComponent<OscillatorComponent>();
	copy->Deserialize(document["OscillatorComponent"]);

	CHECK_FALSE(copy->IsEnabled());
	CheckPosition(copy->GetAxis(), Vector3D(0.0f, 2.0f, 0.0f));
	CHECK(copy->GetAmplitude() == doctest::Approx(3.0f));
	CHECK(copy->GetPeriod() == doctest::Approx(4.0f));
	CHECK(copy->GetPhase() == doctest::Approx(0.5f));
	CheckPosition(copy->GetOrigin(), Vector3D(1.0f, 2.0f, 3.0f));
}

#endif
