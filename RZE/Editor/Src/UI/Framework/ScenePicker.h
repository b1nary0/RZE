#pragma once

#include <Game/World/GameObject/GameObjectDefinitions.h>

#include <Utils/Functor.h>
#include <Utils/Math/Ray.h>
#include <Utils/Math/Vector3D.h>

#include <memory>
#include <optional>

namespace Editor
{
	// What's at a pixel of the main view
	struct ScenePick
	{
		// Null if nothing pickable is there
		GameObjectPtr Object;
		// Where the nearest surface drawn there is, pickable or not; unset where nothing is drawn
		std::optional<Vector3D> SurfacePosition;
		// From the camera through the pixel
		Ray CameraRay;
	};

	// GPU picking in the main view: which GameObject's mesh covers a pixel, and where. Goes through PickRenderStage,
	// so a result takes a frame or two to arrive. Owned by EditorContext; main thread only.
	class ScenePicker
	{
	public:
		using ResultCallback = Functor<void, const ScenePick&>;

	public:
		ScenePicker();
		~ScenePicker();

	public:
		// x and y are relative to the main view's top-left. Replaces any pick still in flight,
		// whose callback is then never called.
		void RequestPick(U32 x, U32 y, const ResultCallback& onResult);
		bool IsPending() const { return m_pending != nullptr; }

		// Delivers the pick in flight if its result has arrived. Once per frame.
		void Update();
		// Drops the pick in flight without calling its callback
		void Cancel();

	private:
		GameObjectPtr FindObject(U32 pickID) const;

	private:
		// The pick in flight, if there is one
		struct PendingPick;
		std::unique_ptr<PendingPick> m_pending;
	};
}
