#pragma once

#include <array>
#include <optional>
#include <string>

class EnumNames;

// How a view smooths jagged edges after tonemapping (AntiAliasingStage)
enum class EAntiAliasingMethod
{
	Off,
	// Fast approximate (FXAA 3.11, Pixel_FXAA.hlsl): blends across the edges it finds in the final image's luma
	FXAA,

	Count
};

// How hard the method works at it
enum class EAntiAliasingQuality
{
	// Only strong edges, the least blur
	Low,
	Medium,
	// Faint edges too, the smoothest
	High,

	Count
};

// A view's anti-aliasing. Engine.ini's [Rendering] AntiAliasing and AntiAliasingQuality set the main view's at
// start-up; the editor's Scene View settings change it while running.
struct AntiAliasingMode
{
	EAntiAliasingMethod Method = EAntiAliasingMethod::FXAA;
	// Unused while the method is Off
	EAntiAliasingQuality Quality = EAntiAliasingQuality::Medium;
};

// What FXAA (3.11 Quality, Pixel_FXAA.hlsl) does at a quality; AntiAliasingStage::FXAAParamsLayout carries it
struct FXAASettings
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

// The FXAA values for a quality, from FXAA 3.11's quality presets (Low 10, Medium 12, High 39)
FXAASettings GetFXAASettings(EAntiAliasingQuality quality);

// Every method's name, as Engine.ini and the editor show it: "Off", "FXAA"
const EnumNames& GetAntiAliasingMethodNames();

const char* GetAntiAliasingMethodName(EAntiAliasingMethod method);

// A method from its name, ignoring case; none if the name isn't one
std::optional<EAntiAliasingMethod> ParseAntiAliasingMethod(const std::string& name);

// Every quality's name, as Engine.ini and the editor show it: "Low", "Medium", "High"
const EnumNames& GetAntiAliasingQualityNames();

const char* GetAntiAliasingQualityName(EAntiAliasingQuality quality);

// A quality from its name, ignoring case; none if the name isn't one
std::optional<EAntiAliasingQuality> ParseAntiAliasingQuality(const std::string& name);
