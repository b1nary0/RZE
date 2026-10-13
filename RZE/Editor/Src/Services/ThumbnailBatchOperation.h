#pragma once

#include <EngineCore/Async/AsyncOperation.h>

#include <Utils/Platform/Filepath.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Editor
{
	class EditorContext;

	// Renders the thumbnails of a list of assets one at a time, in the editor's own scene, then loads the scene that was
	// open before (or a new one if it had no file). Blocking, so the progress modal shows it and nothing else touches
	// the scene meanwhile. A subclass says how to set up and capture one item; a thumbnail that can't be rendered is
	// logged and the batch carries on.
	class ThumbnailBatchOperation : public AsyncOperation
	{
	public:
		ThumbnailBatchOperation(EditorContext& context, const std::string& name, std::vector<Filepath> items);

	protected:
		// Before the first item, e.g. starts a new scene to stage the items in. The first item waits for any scene load
		// this starts.
		virtual void BeginBatch() {}
		// Sets the scene up for item, e.g. loads it
		virtual void BeginItem(const Filepath& item) = 0;
		// Whether the item is set up; it's captured once it has also settled for GetSettleFrames frames
		virtual bool IsItemReady() = 0;
		// Captures the item's thumbnail, calling onDone once with whether it was written
		virtual void CaptureItem(const Filepath& item, std::function<void(bool succeeded)> onDone) = 0;
		// After the item's capture, e.g. removes what BeginItem added
		virtual void EndItem() {}
		// Frames a ready item waits before its capture, for its first frames' work (e.g. the shadow map, uploads)
		virtual int GetSettleFrames() const { return 10; }

		EditorContext& GetContext() { return m_context; }

	private:
		void OnStart() override;
		void OnTick(AsyncFrameBudget& budget) override;
		void OnFinished(EAsyncOperationState finalState) override;

		void StartNextItem();

	private:
		enum class EState
		{
			Preparing,
			WaitingForItem,
			Settling,
			Capturing,
			Restoring
		};

		// Written by a capture's callback, which may outlive the operation
		struct CaptureResult
		{
			bool IsDone = false;
			bool Succeeded = false;
		};

		EditorContext& m_context;
		std::vector<Filepath> m_items;
		size_t m_itemIndex = 0;
		size_t m_failedCount = 0;

		EState m_state = EState::Preparing;
		int m_settleFramesLeft = 0;
		std::shared_ptr<CaptureResult> m_captureResult;

		// The scene to load again at the end; invalid if it had no file
		Filepath m_originalScene;
	};

	namespace ThumbnailBatches
	{
		// Renders each scene's thumbnail through its main camera, loading each in turn: those in scenes, or every
		// scene in the scene folder if it's empty. Asks about unsaved changes first, since it replaces the scene.
		void RenderScenes(EditorContext& context, std::vector<Filepath> scenes = {});

		// Renders every mesh asset's thumbnail, each on its own in a new scene, framed from a three-quarter angle. Asks
		// about unsaved changes first.
		void RenderMeshes(EditorContext& context);
	}
}
