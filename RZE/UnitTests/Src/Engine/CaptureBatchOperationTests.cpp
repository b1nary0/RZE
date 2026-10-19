#include <doctest/doctest.h>

#include <EngineCore/Async/AsyncOperationManager.h>

#include <Game/CaptureBatchOperation.h>

#include <memory>
#include <string>

namespace
{
	// One item that's ready at once, and records when it's captured
	class OneItemBatch final : public CaptureBatchOperation
	{
	public:
		OneItemBatch() : CaptureBatchOperation("Capturing", EAsyncOperationFlags::Background) {}

		bool WasCaptured() const { return m_wasCaptured; }

	protected:
		bool TakeNextItem(Filepath& outItem) override
		{
			if (m_hasTaken)
			{
				return false;
			}
			m_hasTaken = true;
			outItem = Filepath("Assets/UnitTests/Item.scene");
			return true;
		}
		size_t GetItemsLeft() const override { return m_hasTaken ? 0 : 1; }
		void BeginItem(const Filepath&) override {}
		bool IsItemReady() override { return true; }
		void CaptureItem(const Filepath&, std::function<void(bool, const std::string&)> onDone) override
		{
			m_wasCaptured = true;
			onDone(true, std::string());
		}

	private:
		bool m_hasTaken = false;
		bool m_wasCaptured = false;
	};
}

TEST_CASE("A captured item settles for frames, which idle ticks within a frame don't count as")
{
	AsyncOperationManager manager;
	manager.Initialize();

	std::shared_ptr<OneItemBatch> batch = std::make_shared<OneItemBatch>();
	manager.Start(batch);

	// Ready on the first frame, then settles for three more; the frame after captures
	int frames = 0;
	while (!batch->WasCaptured() && frames < 10)
	{
		manager.Tick();
		++frames;
		if (batch->WasCaptured())
		{
			break;
		}

		for (int idleTick = 0; idleTick < 4; ++idleTick)
		{
			manager.TickIdle(5.0);
			REQUIRE_FALSE(batch->WasCaptured());
		}
	}
	CHECK(frames == 5);

	manager.ShutDown();
}
