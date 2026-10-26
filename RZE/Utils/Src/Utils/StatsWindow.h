#pragma once

// Groups frames into windows of k_windowSeconds, so stats averaged over a window update a couple of times a second and
// stay readable. A frame runs from one AddFrame to the next, so the first AddFrame only starts the first window. The
// owner keeps its own sums for the window and turns them into averages when AddFrame closes it.
class StatsWindow
{
public:
	static constexpr double k_windowSeconds = 0.5;

public:
	// Ends the frame at now. True when the window then spans k_windowSeconds: it closes, the getters below describe it,
	// and the next window starts at now.
	bool AddFrame(double now);
	// Whether the first AddFrame has started a window
	bool HasStarted() const { return m_start >= 0.0; }

	// The last closed window's total spread over its frames, e.g. seconds busy to seconds busy per frame. 0 until a
	// window has closed.
	double PerFrame(double total) const;
	// The last closed window's count spread over its seconds, e.g. frames to frames per second. 0 until a window has
	// closed.
	double PerSecond(double count) const;

	// How many frames the last closed window held
	int GetFrameCount() const { return m_closedFrames; }
	// How long the last closed window was, at least k_windowSeconds
	double GetSeconds() const { return m_closedSeconds; }

private:
	// When the current window started; negative before the first AddFrame
	double m_start = -1.0;
	int m_frames = 0;

	int m_closedFrames = 0;
	double m_closedSeconds = 0.0;
};
