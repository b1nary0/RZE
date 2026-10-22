#pragma once

#include <array>
#include <optional>
#include <string>

// How much a view smooths jagged edges after tonemapping (FXAA, PostProcessRenderStage). Engine.ini's
// [Rendering] AntiAliasing sets the main view's at start-up; the editor's Scene View settings change it while running.
enum class EAntiAliasing
{
	Off,
	// Only strong edges, the least blur
	Low,
	Medium,
	// Faint edges too, the smoothest
	High,

	Count
};

// What FXAA (3.11 Quality, Pixel_FXAA.hlsl) does at a level; mirrors the FXAA values in Common/PostProcess.hlsli
struct AntiAliasingSettings
{
	static constexpr int k_maxSearchSteps = 12;

	// Less contrast than max(EdgeThresholdMin, EdgeThreshold x the brightest neighbour's luma) isn't an edge
	float EdgeThreshold = 0.0f;
	float EdgeThresholdMin = 0.0f;
	// How much it softens detail thinner than a pixel (the sparkle on thin parts), 0 to 1
	float Subpixel = 0.0f;
	// The search for each end of an edge, so a long stair-step blends across its whole length: how far each step
	// moves along the edge, in pixels. The first SearchStepCount are used.
	int SearchStepCount = 0;
	std::array<float, k_maxSearchSteps> SearchSteps{};

	// The furthest the search reaches each way, in pixels
	float GetSearchLength() const;
};

// The FXAA values for a level, from FXAA 3.11's quality presets (Low 10, Medium 12, High 39). Off has none; the stage
// skips FXAA.
AntiAliasingSettings GetAntiAliasingSettings(EAntiAliasing level);

// As Engine.ini and the editor show it: "Off", "Low", "Medium", "High"
const char* GetAntiAliasingName(EAntiAliasing level);

// A level from its name, ignoring case; none if the name isn't one
std::optional<EAntiAliasing> ParseAntiAliasing(const std::string& name);
