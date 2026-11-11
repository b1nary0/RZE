#include <doctest/doctest.h>

#include <Rendering/GPUTimer.h>

#include <memory>
#include <set>
#include <vector>

namespace
{
	// The GPU as the fake timestamps see it: what time it is when a timestamp is written, and how far it has got
	struct FakeGPU
	{
		// Seconds; written into each timestamp as it's written
		double Clock = 0.0;
		// Frames are numbered as they begin; the GPU has finished those below this
		U32 FinishedFrames = 0;
		// Frames whose times can't be trusted (D3D11's disjoint clock)
		std::set<U32> InvalidFrames;
		U32 NextFrame = 0;
		U32 Begins = 0;
		// Set by any Write past the capacity
		bool WroteOutOfRange = false;
	};

	class FakeTimestamps : public Rendering::IGPUTimestamps
	{
	public:
		FakeTimestamps(FakeGPU& gpu, U32 capacity)
			: m_gpu(gpu)
			, m_times(capacity, -1.0)
		{
		}

		void Begin() override
		{
			m_frame = m_gpu.NextFrame++;
			++m_gpu.Begins;
			std::fill(m_times.begin(), m_times.end(), -1.0);
		}

		void Write(U32 index) override
		{
			if (index >= m_times.size())
			{
				m_gpu.WroteOutOfRange = true;
				return;
			}
			m_times[index] = m_gpu.Clock;
		}

		void End() override
		{
		}

		Rendering::EGPUTimestampsReadResult Read(U32 count, std::vector<double>& outSeconds) override
		{
			if (m_frame >= m_gpu.FinishedFrames)
			{
				return Rendering::EGPUTimestampsReadResult::NotReady;
			}
			if (m_gpu.InvalidFrames.count(m_frame) > 0)
			{
				return Rendering::EGPUTimestampsReadResult::Invalid;
			}

			outSeconds.resize(count);
			for (U32 index = 0; index < count; ++index)
			{
				outSeconds[index] = m_times[index] - m_times[0];
			}
			return Rendering::EGPUTimestampsReadResult::Ready;
		}

	private:
		FakeGPU& m_gpu;
		std::vector<double> m_times;
		U32 m_frame = 0;
	};

	Rendering::GPUTimestampsFactory MakeFactory(FakeGPU& gpu)
	{
		return [&gpu](U32 capacity) { return std::make_unique<FakeTimestamps>(gpu, capacity); };
	}

	// One frame, its first timestamp at the GPU's clock as it is: nothing timed but the frame itself, which takes
	// frameSeconds
	bool RunEmptyFrame(Rendering::GPUTimer& timer, FakeGPU& gpu, double frameSeconds, Rendering::GPUFrameTimes& outTimes)
	{
		timer.BeginFrameIfNeeded();
		gpu.Clock += frameSeconds;
		return timer.EndFrame(outTimes);
	}
}

TEST_CASE("A frame's scopes are summed by id and read back once the GPU has finished it")
{
	FakeGPU gpu;
	Rendering::GPUTimer timer;
	timer.Initialize(MakeFactory(gpu));

	timer.BeginFrameIfNeeded();
	gpu.Clock = 1.0;
	timer.BeginScope(2);
	gpu.Clock = 4.0;
	timer.EndScope();
	gpu.Clock = 5.0;
	timer.BeginScope(2);
	gpu.Clock = 6.0;
	timer.EndScope();
	gpu.Clock = 10.0;

	Rendering::GPUFrameTimes times;
	// The GPU hasn't finished it, and nothing before it was timed
	CHECK_FALSE(timer.EndFrame(times));

	gpu.FinishedFrames = 1;
	REQUIRE(RunEmptyFrame(timer, gpu, 1.0, times));
	CHECK(times.FrameMS == doctest::Approx(10000.0));
	REQUIRE(times.ScopeMS.size() == 3);
	CHECK(times.ScopeMS[0] == 0.0);
	CHECK(times.ScopeMS[1] == 0.0);
	CHECK(times.ScopeMS[2] == doctest::Approx(4000.0));

	timer.Release();
}

TEST_CASE("A nested scope's time is part of the scope around it")
{
	FakeGPU gpu;
	Rendering::GPUTimer timer;
	timer.Initialize(MakeFactory(gpu));

	timer.BeginFrameIfNeeded();
	timer.BeginScope(0);
	gpu.Clock = 1.0;
	timer.BeginScope(1);
	gpu.Clock = 3.0;
	timer.EndScope();
	gpu.Clock = 4.0;
	timer.EndScope();

	Rendering::GPUFrameTimes times;
	CHECK_FALSE(timer.EndFrame(times));
	gpu.FinishedFrames = 1;
	REQUIRE(RunEmptyFrame(timer, gpu, 1.0, times));
	REQUIRE(times.ScopeMS.size() == 2);
	CHECK(times.ScopeMS[0] == doctest::Approx(4000.0));
	CHECK(times.ScopeMS[1] == doctest::Approx(2000.0));

	timer.Release();
}

TEST_CASE("Frames go untimed while the GPU is a whole ring of frames behind, and timing picks up after")
{
	FakeGPU gpu;
	Rendering::GPUTimer timer;
	timer.Initialize(MakeFactory(gpu));

	Rendering::GPUFrameTimes times;
	for (size_t frame = 0; frame < Rendering::GPUTimer::k_frameCount; ++frame)
	{
		CHECK_FALSE(RunEmptyFrame(timer, gpu, 1.0, times));
	}
	CHECK(gpu.Begins == Rendering::GPUTimer::k_frameCount);

	// Every set is waiting on the GPU: this frame isn't timed, and its scopes are ignored
	timer.BeginFrameIfNeeded();
	timer.BeginScope(0);
	timer.EndScope();
	CHECK_FALSE(timer.EndFrame(times));
	CHECK(gpu.Begins == Rendering::GPUTimer::k_frameCount);

	// The GPU catches up: the oldest frames are read back, newest last
	gpu.FinishedFrames = 2;
	REQUIRE(RunEmptyFrame(timer, gpu, 3.0, times));
	CHECK(times.FrameMS == doctest::Approx(1000.0));
	// Read back as that frame presented, after it began untimed: the next frame is the first timed again
	CHECK(gpu.Begins == Rendering::GPUTimer::k_frameCount);
	RunEmptyFrame(timer, gpu, 1.0, times);
	CHECK(gpu.Begins == Rendering::GPUTimer::k_frameCount + 1);

	timer.Release();
}

TEST_CASE("Scopes that don't fit in a frame's timestamps aren't timed, and the frame still is")
{
	FakeGPU gpu;
	Rendering::GPUTimer timer;
	timer.Initialize(MakeFactory(gpu));

	timer.BeginFrameIfNeeded();
	const U32 scopeCount = Rendering::GPUTimer::k_capacity;
	for (U32 scope = 0; scope < scopeCount; ++scope)
	{
		timer.BeginScope(scope);
		gpu.Clock += 1.0;
		timer.EndScope();
	}

	Rendering::GPUFrameTimes times;
	CHECK_FALSE(timer.EndFrame(times));
	CHECK_FALSE(gpu.WroteOutOfRange);

	gpu.FinishedFrames = 1;
	REQUIRE(RunEmptyFrame(timer, gpu, 1.0, times));
	CHECK(times.FrameMS == doctest::Approx(scopeCount * 1000.0));
	// Two timestamps a scope, and the frame's first and last
	const U32 timedScopes = (Rendering::GPUTimer::k_capacity - 2) / 2;
	REQUIRE(times.ScopeMS.size() == timedScopes);
	CHECK(times.ScopeMS.back() == doctest::Approx(1000.0));

	timer.Release();
}

TEST_CASE("A frame whose times can't be trusted is skipped")
{
	FakeGPU gpu;
	Rendering::GPUTimer timer;
	timer.Initialize(MakeFactory(gpu));

	Rendering::GPUFrameTimes times;
	CHECK_FALSE(RunEmptyFrame(timer, gpu, 1.0, times));
	CHECK_FALSE(RunEmptyFrame(timer, gpu, 2.0, times));

	gpu.InvalidFrames.insert(1);
	gpu.FinishedFrames = 2;
	// Frame 0 reads back, frame 1 is skipped, so the newest read is frame 0's
	REQUIRE(RunEmptyFrame(timer, gpu, 3.0, times));
	CHECK(times.FrameMS == doctest::Approx(1000.0));

	gpu.FinishedFrames = 3;
	REQUIRE(RunEmptyFrame(timer, gpu, 4.0, times));
	CHECK(times.FrameMS == doctest::Approx(3000.0));

	timer.Release();
}
