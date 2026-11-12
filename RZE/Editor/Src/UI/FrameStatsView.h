#pragma once

#include <imgui/imgui.h>

namespace Editor
{
	// The frame stats the status bar shows (StatusBar): a line of figures in the bar, and the details of the frame in
	// tables for its hover panel. Every figure is drawn against the right edge of a slot as wide as the widest it usually
	// gets, so the figures and what follows them stay put as the values change.
	//
	// The details: the main thread's frame and idle time (FrameStats); the draw calls, instances and recording time
	// (RenderStats), then a table of the render stages, each with its record time, GPU time, draws and instances, and
	// what the GPU counted in it (triangles submitted, vertices shaded, triangles in view and pixels shaded), with a
	// total; the scene's probe and shadow work; and the render thread's busy time and the GPU's frame time
	// (Rendering::RenderThreadStats). Every table spans the panel, with its figures against the right edge. The GPU
	// only counts while the details are shown (RenderEngine::RequestGPUStatistics): counting costs GPU time.
	namespace FrameStatsView
	{
		// The line of figures, its text's top left at position
		void DrawLine(const ImVec2& position);
		// The details, into the current window (the status bar's hover panel). Asks the GPU to count while it's called.
		void DrawDetails();
	}
}
