#pragma once

#include <Game/World/GameObject/GameObjectDefinitions.h>

#include <Utils/Functor.h>

#include <memory>

namespace Rendering
{
	struct TexelReadback;
}

namespace Editor
{
	// GPU picking in the main view: which GameObject's mesh covers a pixel. Goes through PickRenderStage,
	// so a result takes a frame or two to arrive. Owned by EditorContext; main thread only.
	class ScenePicker
	{
	public:
		// The hit object, or null if nothing pickable is at the pixel
		using ResultCallback = Functor<void, GameObjectPtr>;

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
		std::shared_ptr<Rendering::TexelReadback> m_pendingResult;
		ResultCallback m_onResult;
	};
}
