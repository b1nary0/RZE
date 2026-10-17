#pragma once

#include <Game/World/GameObject/GameObjectDefinitions.h>

#include <Utils/Functor.h>
#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Ray.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>

#include <memory>
#include <optional>

namespace Rendering
{
	struct TexelReadback;
}

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
		ScenePicker() = default;
		~ScenePicker() = default;

	public:
		// x and y are relative to the main view's top-left. Replaces any pick still in flight,
		// whose callback is then never called.
		void RequestPick(U32 x, U32 y, const ResultCallback& onResult);
		bool IsPending() const { return m_pendingResult != nullptr; }

		// Delivers the pick in flight if its result has arrived. Once per frame.
		void Update();
		// Drops the pick in flight without calling its callback
		void Cancel();

	private:
		GameObjectPtr FindObject(U32 pickID) const;

	private:
		std::shared_ptr<Rendering::TexelReadback> m_pendingResult;
		std::shared_ptr<Rendering::TexelReadback> m_pendingDepth;
		ResultCallback m_onResult;

		// The pixel's centre, and the view's camera and size when it was asked for, to turn the depth into a position
		Vector2D m_pixel;
		Matrix4x4 m_clipSpace;
		Vector2D m_viewSize;
	};
}
