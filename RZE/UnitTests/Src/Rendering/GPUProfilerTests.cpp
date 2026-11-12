#include <doctest/doctest.h>

#include <Rendering/GPUProfiler.h>

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

		// What the GPU has counted so far, as its pipeline statistics counters run: Draw adds to it
		Rendering::GPUPipelineStatistics Counters;
		// Frames whose statistics aren't ready even once the frame has finished, and frames whose statistics can't be read
		std::set<U32> LateStatisticsFrames;
		std::set<U32> InvalidStatisticsFrames;
		U32 StatisticsBegins = 0;
		U32 SegmentsBegun = 0;
		// Set by a segment begun while another is open, or past the capacity
		bool OverlappedSegments = false;
		bool SegmentOutOfRange = false;

		// A draw of triangleCount triangles, with distinct counts for each statistic so a mix-up shows
		void Draw(double triangleCount)
		{
			Counters.TrianglesSubmitted += triangleCount;
			Counters.VerticesShaded += triangleCount * 2.0;
			Counters.TrianglesInView += triangleCount * 0.5;
			Counters.PixelsShaded += triangleCount * 10.0;
		}
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

		Rendering::EGPUQueryReadResult Read(U32 count, std::vector<double>& outSeconds) override
		{
			if (m_frame >= m_gpu.FinishedFrames)
			{
				return Rendering::EGPUQueryReadResult::NotReady;
			}
			if (m_gpu.InvalidFrames.count(m_frame) > 0)
			{
				return Rendering::EGPUQueryReadResult::Invalid;
			}

			outSeconds.resize(count);
			for (U32 index = 0; index < count; ++index)
			{
				outSeconds[index] = m_times[index] - m_times[0];
			}
			return Rendering::EGPUQueryReadResult::Ready;
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

	// Counts like a GPU's statistics queries: each segment is what the counters went up by between its begin and end
	class FakeStatistics : public Rendering::IGPUStatistics
	{
	public:
		FakeStatistics(FakeGPU& gpu, U32 capacity)
			: m_gpu(gpu)
			, m_segments(capacity)
		{
		}

		void Begin() override
		{
			// The timestamps begin the frame first, which numbered it
			m_frame = m_gpu.NextFrame - 1;
			++m_gpu.StatisticsBegins;
		}

		void BeginSegment(U32 index) override
		{
			++m_gpu.SegmentsBegun;
			if (m_isOpen)
			{
				m_gpu.OverlappedSegments = true;
			}
			if (index >= m_segments.size())
			{
				m_gpu.SegmentOutOfRange = true;
				return;
			}
			m_isOpen = true;
			m_open = index;
			m_atBegin = m_gpu.Counters;
		}

		void EndSegment() override
		{
			if (!m_isOpen)
			{
				return;
			}
			const Rendering::GPUPipelineStatistics& now = m_gpu.Counters;
			Rendering::GPUPipelineStatistics& segment = m_segments[m_open];
			segment.TrianglesSubmitted = now.TrianglesSubmitted - m_atBegin.TrianglesSubmitted;
			segment.VerticesShaded = now.VerticesShaded - m_atBegin.VerticesShaded;
			segment.TrianglesInView = now.TrianglesInView - m_atBegin.TrianglesInView;
			segment.PixelsShaded = now.PixelsShaded - m_atBegin.PixelsShaded;
			m_isOpen = false;
		}

		void End() override
		{
		}

		Rendering::EGPUQueryReadResult Read(U32 count, std::vector<Rendering::GPUPipelineStatistics>& outSegments) override
		{
			if (m_frame >= m_gpu.FinishedFrames || m_gpu.LateStatisticsFrames.count(m_frame) > 0)
			{
				return Rendering::EGPUQueryReadResult::NotReady;
			}
			if (m_gpu.InvalidStatisticsFrames.count(m_frame) > 0)
			{
				return Rendering::EGPUQueryReadResult::Invalid;
			}
			outSegments.assign(m_segments.begin(), m_segments.begin() + count);
			return Rendering::EGPUQueryReadResult::Ready;
		}

	private:
		FakeGPU& m_gpu;
		std::vector<Rendering::GPUPipelineStatistics> m_segments;
		Rendering::GPUPipelineStatistics m_atBegin;
		U32 m_frame = 0;
		U32 m_open = 0;
		bool m_isOpen = false;
	};

	Rendering::GPUStatisticsFactory MakeStatisticsFactory(FakeGPU& gpu)
	{
		return [&gpu](U32 capacity) { return std::make_unique<FakeStatistics>(gpu, capacity); };
	}

	// One frame, its first timestamp at the GPU's clock as it is: nothing timed but the frame itself, which takes
	// frameSeconds
	bool RunEmptyFrame(Rendering::GPUProfiler& profiler, FakeGPU& gpu, double frameSeconds, Rendering::GPUFrameProfile& outProfile)
	{
		profiler.BeginFrameIfNeeded();
		gpu.Clock += frameSeconds;
		return profiler.EndFrame(outProfile);
	}
}

TEST_CASE("A frame's scopes are summed by id and read back once the GPU has finished it")
{
	FakeGPU gpu;
	Rendering::GPUProfiler profiler;
	profiler.Initialize(MakeFactory(gpu), nullptr);

	profiler.BeginFrameIfNeeded();
	gpu.Clock = 1.0;
	profiler.BeginScope(2);
	gpu.Clock = 4.0;
	profiler.EndScope();
	gpu.Clock = 5.0;
	profiler.BeginScope(2);
	gpu.Clock = 6.0;
	profiler.EndScope();
	gpu.Clock = 10.0;

	Rendering::GPUFrameProfile profile;
	// The GPU hasn't finished it, and nothing before it was timed
	CHECK_FALSE(profiler.EndFrame(profile));

	gpu.FinishedFrames = 1;
	REQUIRE(RunEmptyFrame(profiler, gpu, 1.0, profile));
	CHECK(profile.FrameMS == doctest::Approx(10000.0));
	REQUIRE(profile.ScopeMS.size() == 3);
	CHECK(profile.ScopeMS[0] == 0.0);
	CHECK(profile.ScopeMS[1] == 0.0);
	CHECK(profile.ScopeMS[2] == doctest::Approx(4000.0));

	profiler.Release();
}

TEST_CASE("A nested scope's time is part of the scope around it")
{
	FakeGPU gpu;
	Rendering::GPUProfiler profiler;
	profiler.Initialize(MakeFactory(gpu), nullptr);

	profiler.BeginFrameIfNeeded();
	profiler.BeginScope(0);
	gpu.Clock = 1.0;
	profiler.BeginScope(1);
	gpu.Clock = 3.0;
	profiler.EndScope();
	gpu.Clock = 4.0;
	profiler.EndScope();

	Rendering::GPUFrameProfile profile;
	CHECK_FALSE(profiler.EndFrame(profile));
	gpu.FinishedFrames = 1;
	REQUIRE(RunEmptyFrame(profiler, gpu, 1.0, profile));
	REQUIRE(profile.ScopeMS.size() == 2);
	CHECK(profile.ScopeMS[0] == doctest::Approx(4000.0));
	CHECK(profile.ScopeMS[1] == doctest::Approx(2000.0));

	profiler.Release();
}

TEST_CASE("Frames go untimed while the GPU is a whole ring of frames behind, and timing picks up after")
{
	FakeGPU gpu;
	Rendering::GPUProfiler profiler;
	profiler.Initialize(MakeFactory(gpu), nullptr);

	Rendering::GPUFrameProfile profile;
	for (size_t frame = 0; frame < Rendering::GPUProfiler::k_frameCount; ++frame)
	{
		CHECK_FALSE(RunEmptyFrame(profiler, gpu, 1.0, profile));
	}
	CHECK(gpu.Begins == Rendering::GPUProfiler::k_frameCount);

	// Every set is waiting on the GPU: this frame isn't timed, and its scopes are ignored
	profiler.BeginFrameIfNeeded();
	profiler.BeginScope(0);
	profiler.EndScope();
	CHECK_FALSE(profiler.EndFrame(profile));
	CHECK(gpu.Begins == Rendering::GPUProfiler::k_frameCount);

	// The GPU catches up: the oldest frames are read back, newest last
	gpu.FinishedFrames = 2;
	REQUIRE(RunEmptyFrame(profiler, gpu, 3.0, profile));
	CHECK(profile.FrameMS == doctest::Approx(1000.0));
	// Read back as that frame presented, after it began untimed: the next frame is the first timed again
	CHECK(gpu.Begins == Rendering::GPUProfiler::k_frameCount);
	RunEmptyFrame(profiler, gpu, 1.0, profile);
	CHECK(gpu.Begins == Rendering::GPUProfiler::k_frameCount + 1);

	profiler.Release();
}

TEST_CASE("Scopes that don't fit in a frame's timestamps aren't timed, and the frame still is")
{
	FakeGPU gpu;
	Rendering::GPUProfiler profiler;
	profiler.Initialize(MakeFactory(gpu), nullptr);

	profiler.BeginFrameIfNeeded();
	const U32 scopeCount = Rendering::GPUProfiler::k_capacity;
	for (U32 scope = 0; scope < scopeCount; ++scope)
	{
		profiler.BeginScope(scope);
		gpu.Clock += 1.0;
		profiler.EndScope();
	}

	Rendering::GPUFrameProfile profile;
	CHECK_FALSE(profiler.EndFrame(profile));
	CHECK_FALSE(gpu.WroteOutOfRange);

	gpu.FinishedFrames = 1;
	REQUIRE(RunEmptyFrame(profiler, gpu, 1.0, profile));
	CHECK(profile.FrameMS == doctest::Approx(scopeCount * 1000.0));
	// Two timestamps a scope, and the frame's first and last
	const U32 timedScopes = (Rendering::GPUProfiler::k_capacity - 2) / 2;
	REQUIRE(profile.ScopeMS.size() == timedScopes);
	CHECK(profile.ScopeMS.back() == doctest::Approx(1000.0));

	profiler.Release();
}

TEST_CASE("A frame whose times can't be trusted is skipped")
{
	FakeGPU gpu;
	Rendering::GPUProfiler profiler;
	profiler.Initialize(MakeFactory(gpu), nullptr);

	Rendering::GPUFrameProfile profile;
	CHECK_FALSE(RunEmptyFrame(profiler, gpu, 1.0, profile));
	CHECK_FALSE(RunEmptyFrame(profiler, gpu, 2.0, profile));

	gpu.InvalidFrames.insert(1);
	gpu.FinishedFrames = 2;
	// Frame 0 reads back, frame 1 is skipped, so the newest read is frame 0's
	REQUIRE(RunEmptyFrame(profiler, gpu, 3.0, profile));
	CHECK(profile.FrameMS == doctest::Approx(1000.0));

	gpu.FinishedFrames = 3;
	REQUIRE(RunEmptyFrame(profiler, gpu, 4.0, profile));
	CHECK(profile.FrameMS == doctest::Approx(3000.0));

	profiler.Release();
}

TEST_CASE("Each scope counts what the GPU did inside it, and nothing between scopes is counted")
{
	FakeGPU gpu;
	Rendering::GPUProfiler profiler;
	profiler.Initialize(MakeFactory(gpu), MakeStatisticsFactory(gpu));
	REQUIRE(profiler.HasStatistics());

	profiler.BeginFrameIfNeeded();
	gpu.Draw(1000.0);
	profiler.BeginScope(0);
	gpu.Draw(100.0);
	profiler.EndScope();
	// Between scopes, like the UI: no segment is open
	gpu.Draw(7.0);
	profiler.BeginScope(2);
	gpu.Draw(30.0);
	profiler.EndScope();

	Rendering::GPUFrameProfile profile;
	CHECK_FALSE(profiler.EndFrame(profile));
	CHECK(gpu.SegmentsBegun == 2);
	CHECK_FALSE(gpu.OverlappedSegments);

	gpu.FinishedFrames = 1;
	REQUIRE(RunEmptyFrame(profiler, gpu, 1.0, profile));
	REQUIRE(profile.HasStatistics);
	REQUIRE(profile.ScopeStatistics.size() == 3);
	CHECK(profile.ScopeStatistics[0].TrianglesSubmitted == doctest::Approx(100.0));
	CHECK(profile.ScopeStatistics[0].VerticesShaded == doctest::Approx(200.0));
	CHECK(profile.ScopeStatistics[0].TrianglesInView == doctest::Approx(50.0));
	CHECK(profile.ScopeStatistics[0].PixelsShaded == doctest::Approx(1000.0));
	CHECK(profile.ScopeStatistics[1].TrianglesSubmitted == 0.0);
	CHECK(profile.ScopeStatistics[2].TrianglesSubmitted == doctest::Approx(30.0));
	CHECK(profile.FrameStatistics.TrianglesSubmitted == doctest::Approx(130.0));

	profiler.Release();
}

TEST_CASE("A nested scope's counts are part of the scope around it, and segments never overlap")
{
	FakeGPU gpu;
	Rendering::GPUProfiler profiler;
	profiler.Initialize(MakeFactory(gpu), MakeStatisticsFactory(gpu));

	profiler.BeginFrameIfNeeded();
	profiler.BeginScope(0);
	gpu.Draw(5.0);
	profiler.BeginScope(1);
	gpu.Draw(10.0);
	profiler.EndScope();
	gpu.Draw(3.0);
	profiler.EndScope();

	Rendering::GPUFrameProfile profile;
	CHECK_FALSE(profiler.EndFrame(profile));
	// Before the inner scope, the inner scope, after it
	CHECK(gpu.SegmentsBegun == 3);
	CHECK_FALSE(gpu.OverlappedSegments);

	gpu.FinishedFrames = 1;
	REQUIRE(RunEmptyFrame(profiler, gpu, 1.0, profile));
	REQUIRE(profile.ScopeStatistics.size() == 2);
	CHECK(profile.ScopeStatistics[0].TrianglesSubmitted == doctest::Approx(18.0));
	CHECK(profile.ScopeStatistics[1].TrianglesSubmitted == doctest::Approx(10.0));
	// Counted once, though the inner scope's draws are in both
	CHECK(profile.FrameStatistics.TrianglesSubmitted == doctest::Approx(18.0));

	profiler.Release();
}

TEST_CASE("A frame whose statistics aren't ready waits for them, though its times are")
{
	FakeGPU gpu;
	Rendering::GPUProfiler profiler;
	profiler.Initialize(MakeFactory(gpu), MakeStatisticsFactory(gpu));

	profiler.BeginFrameIfNeeded();
	profiler.BeginScope(0);
	gpu.Draw(4.0);
	profiler.EndScope();
	Rendering::GPUFrameProfile profile;
	CHECK_FALSE(profiler.EndFrame(profile));

	gpu.FinishedFrames = 1;
	gpu.LateStatisticsFrames.insert(0);
	CHECK_FALSE(RunEmptyFrame(profiler, gpu, 1.0, profile));

	gpu.LateStatisticsFrames.clear();
	REQUIRE(RunEmptyFrame(profiler, gpu, 1.0, profile));
	CHECK(profile.ScopeStatistics[0].TrianglesSubmitted == doctest::Approx(4.0));

	profiler.Release();
}

TEST_CASE("A frame whose statistics can't be read is skipped, times and all")
{
	FakeGPU gpu;
	Rendering::GPUProfiler profiler;
	profiler.Initialize(MakeFactory(gpu), MakeStatisticsFactory(gpu));

	Rendering::GPUFrameProfile profile;
	CHECK_FALSE(RunEmptyFrame(profiler, gpu, 1.0, profile));
	CHECK_FALSE(RunEmptyFrame(profiler, gpu, 2.0, profile));

	gpu.InvalidStatisticsFrames.insert(1);
	gpu.FinishedFrames = 2;
	// Frame 0 reads back, frame 1 is skipped, so the newest read is frame 0's
	REQUIRE(RunEmptyFrame(profiler, gpu, 3.0, profile));
	CHECK(profile.FrameMS == doctest::Approx(1000.0));

	profiler.Release();
}

TEST_CASE("Without statistics, frames are timed as before and report none")
{
	FakeGPU gpu;
	Rendering::GPUProfiler profiler;
	// A device that can't count them
	profiler.Initialize(MakeFactory(gpu), [](U32) { return std::unique_ptr<Rendering::IGPUStatistics>(); });
	CHECK_FALSE(profiler.HasStatistics());

	profiler.BeginFrameIfNeeded();
	profiler.BeginScope(0);
	gpu.Clock = 2.0;
	gpu.Draw(9.0);
	profiler.EndScope();
	Rendering::GPUFrameProfile profile;
	CHECK_FALSE(profiler.EndFrame(profile));

	gpu.FinishedFrames = 1;
	REQUIRE(RunEmptyFrame(profiler, gpu, 1.0, profile));
	CHECK_FALSE(profile.HasStatistics);
	CHECK(profile.ScopeMS[0] == doctest::Approx(2000.0));
	CHECK(profile.FrameStatistics.TrianglesSubmitted == 0.0);
	CHECK(gpu.StatisticsBegins == 0);

	profiler.Release();
}

TEST_CASE("Frames skipped while the GPU is behind are skipped for statistics too")
{
	FakeGPU gpu;
	Rendering::GPUProfiler profiler;
	profiler.Initialize(MakeFactory(gpu), MakeStatisticsFactory(gpu));

	Rendering::GPUFrameProfile profile;
	for (size_t frame = 0; frame <= Rendering::GPUProfiler::k_frameCount; ++frame)
	{
		profiler.BeginFrameIfNeeded();
		profiler.BeginScope(0);
		gpu.Draw(1.0);
		profiler.EndScope();
		CHECK_FALSE(profiler.EndFrame(profile));
	}
	// The last frame had no free set: neither its timestamps nor its statistics began
	CHECK(gpu.Begins == Rendering::GPUProfiler::k_frameCount);
	CHECK(gpu.StatisticsBegins == Rendering::GPUProfiler::k_frameCount);
	CHECK(gpu.SegmentsBegun == Rendering::GPUProfiler::k_frameCount);

	profiler.Release();
}

TEST_CASE("A scope that doesn't fit is counted in the scope around it, and segments stay within capacity")
{
	FakeGPU gpu;
	Rendering::GPUProfiler profiler;
	profiler.Initialize(MakeFactory(gpu), MakeStatisticsFactory(gpu));

	profiler.BeginFrameIfNeeded();
	profiler.BeginScope(0);
	const U32 innerScopes = Rendering::GPUProfiler::k_capacity;
	for (U32 scope = 1; scope <= innerScopes; ++scope)
	{
		profiler.BeginScope(scope);
		gpu.Draw(1.0);
		profiler.EndScope();
	}
	profiler.EndScope();

	Rendering::GPUFrameProfile profile;
	CHECK_FALSE(profiler.EndFrame(profile));
	CHECK_FALSE(gpu.WroteOutOfRange);
	CHECK_FALSE(gpu.SegmentOutOfRange);
	CHECK_FALSE(gpu.OverlappedSegments);

	gpu.FinishedFrames = 1;
	REQUIRE(RunEmptyFrame(profiler, gpu, 1.0, profile));
	// Every inner scope's draw is in the outer one, profiled or not
	CHECK(profile.ScopeStatistics[0].TrianglesSubmitted == doctest::Approx(innerScopes));
	CHECK(profile.FrameStatistics.TrianglesSubmitted == doctest::Approx(innerScopes));
	// Those that fit have their own; the outer scope's two timestamps and the frame's leave room for the rest
	const U32 profiledInner = (Rendering::GPUProfiler::k_capacity - 4) / 2;
	REQUIRE(profile.ScopeStatistics.size() == profiledInner + 1);
	CHECK(profile.ScopeStatistics.back().TrianglesSubmitted == doctest::Approx(1.0));

	profiler.Release();
}
