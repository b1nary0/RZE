#pragma once

#include <optional>
#include <string>

class EnumNames;

// How much a view darkens the indirect light in creases and corners (GTAO, AmbientOcclusionStage). Engine.ini's
// [Rendering] AmbientOcclusion sets the main view's at start-up; the editor's Graphics settings change it while running.
enum class EAmbientOcclusion
{
	Off,
	// The fewest directions per pixel, the noisiest
	Low,
	Medium,
	// The most directions and steps, the smoothest
	High,

	Count
};

// What GTAO (a port of Intel's XeGTAO) does at a level: its quality presets
struct AmbientOcclusionSettings
{
	// Directions searched around each pixel for the horizon, and how many depths each direction reads per side
	int SliceCount = 0;
	int StepsPerSlice = 0;
	// Edge-aware blurs over the result; the noise is fixed per pixel (no TAA to average it over frames)
	int DenoisePasses = 0;
};

// The GTAO values for a level, from XeGTAO's Low, Medium and High presets. Off has none; the stage skips GTAO.
AmbientOcclusionSettings GetAmbientOcclusionSettings(EAmbientOcclusion level);

// Every level's name, as Engine.ini and the editor show it: "Off", "Low", "Medium", "High"
const EnumNames& GetAmbientOcclusionNames();

const char* GetAmbientOcclusionName(EAmbientOcclusion level);

// A level from its name, ignoring case; none if the name isn't one
std::optional<EAmbientOcclusion> ParseAmbientOcclusion(const std::string& name);
