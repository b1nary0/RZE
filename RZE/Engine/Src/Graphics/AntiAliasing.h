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
	// Subpixel morphological (SMAA 1x, Common/SMAA): finds each edge's shape, so it blends only across edges and
	// keeps texture detail sharp; handles diagonals and corners from High. Three passes.
	SMAA,

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
	// SMAA only: the faintest edges and the longest searches. FXAA uses High's.
	Ultra,

	Count
};

// The highest quality the method has: Ultra for SMAA, High for FXAA (and for Off, which has no quality to use)
EAntiAliasingQuality GetHighestQuality(EAntiAliasingMethod method);

// A view's anti-aliasing. Engine.ini's [Rendering] AntiAliasing and AntiAliasingQuality set the main view's at
// start-up; the editor's Scene View settings change it while running.
struct AntiAliasingMode
{
	EAntiAliasingMethod Method = EAntiAliasingMethod::SMAA;
	// Unused while the method is Off. High is SMAA's first quality that follows diagonals and keeps corners.
	EAntiAliasingQuality Quality = EAntiAliasingQuality::High;
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

// The FXAA values for a quality, from FXAA 3.11's quality presets (Low 10, Medium 12, High 39). Ultra is High.
FXAASettings GetFXAASettings(EAntiAliasingQuality quality);

// What SMAA (Common/SMAA/SMAA.hlsl) does at a quality; AntiAliasingStage::SMAAParamsLayout carries it
struct SMAASettings
{
	// Less luma contrast than this isn't an edge
	float Threshold = 0.0f;
	// How far the search for each end of an edge goes, in steps of 2 pixels
	int SearchSteps = 0;
	// Whether it finds diagonal edges and keeps sharp corners (Pixel_SMAABlendingWeights), or only horizontal and
	// vertical edges (Pixel_SMAABlendingWeightsNoDiagonals)
	bool DetectsDiagonalsAndCorners = false;
	// The diagonal search's length, in steps; with DetectsDiagonalsAndCorners only
	int DiagonalSearchSteps = 0;
	// How much a sharp corner is kept from being rounded off, 0 to 100; with DetectsDiagonalsAndCorners only
	float CornerRounding = 0.0f;
};

// The SMAA values for a quality: SMAA.hlsl's presets, SMAA_PRESET_LOW to SMAA_PRESET_ULTRA
SMAASettings GetSMAASettings(EAntiAliasingQuality quality);

// Every method's name, as Engine.ini and the editor show it: "Off", "FXAA", "SMAA"
const EnumNames& GetAntiAliasingMethodNames();

const char* GetAntiAliasingMethodName(EAntiAliasingMethod method);

// A method from its name, ignoring case; none if the name isn't one
std::optional<EAntiAliasingMethod> ParseAntiAliasingMethod(const std::string& name);

// Every quality's name, as Engine.ini and the editor show it: "Low", "Medium", "High", "Ultra"
const EnumNames& GetAntiAliasingQualityNames();

const char* GetAntiAliasingQualityName(EAntiAliasingQuality quality);

// A quality from its name, ignoring case; none if the name isn't one
std::optional<EAntiAliasingQuality> ParseAntiAliasingQuality(const std::string& name);
