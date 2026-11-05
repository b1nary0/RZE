#include <EngineCore/Config/EngineConfig.h>

#include <Utils/Conversions.h>

#include <algorithm>

EngineConfig::EngineConfig()
{

}

EngineConfig::~EngineConfig()
{

}

bool EngineConfig::Load(const Filepath& filePath)
{
	Config::Read(filePath);

	LoadEngineSettings();
	LoadWindowSettings();
	LoadFrameSettings();
	LoadBackgroundTaskFilter();
	LoadRenderingSettings();

	// @TODO properly return true/false from this function. for now true is fine but jeapordizes the integrity of the resource system
	return true;
}

void EngineConfig::Release()
{
}

const EngineSettings& EngineConfig::GetEngineSettings() const
{
	return mEngineSettings;
}

const WindowSettings& EngineConfig::GetWindowSettings() const
{
	return mWindowSettings;
}

const FrameSettings& EngineConfig::GetFrameSettings() const
{
	return mFrameSettings;
}

const AsyncOperationFilter& EngineConfig::GetBackgroundTaskFilter() const
{
	return mBackgroundTaskFilter;
}

void EngineConfig::LoadEngineSettings()
{
	mEngineSettings.mMSAASampleCount = Conversions::IntFromString(mINIParser.GetValue("EngineSettings", "MSAASampleCount", "1"));
}

void EngineConfig::LoadWindowSettings()
{
	int width = 0;
	int height = 0;
	width = Conversions::IntFromString(mINIParser.GetValue("WindowSettings", "WindowWidth", "1280"));
	height = Conversions::IntFromString(mINIParser.GetValue("WindowSettings", "WindowHeight", "768"));

	mWindowSettings.mDimensions = Vector2D(width, height);
	mWindowSettings.mTitle = mINIParser.GetValue("WindowSettings", "WindowTitle", "RZE_Engine");
	mWindowSettings.mLaunchMonitor = Conversions::IntFromString(mINIParser.GetValue("WindowSettings", "LaunchMonitor", "0"));
}

void EngineConfig::LoadFrameSettings()
{
	// Missing keys keep FrameSettings' defaults
	const FrameSettings defaults;
	mFrameSettings.VSync = mINIParser.GetBoolValue("FrameSettings", "EnableVsync", defaults.VSync);
	mFrameSettings.MaxFrameRate = mINIParser.GetDoubleValue("FrameSettings", "MaxFrameRate", defaults.MaxFrameRate);
	mFrameSettings.UnfocusedFrameRate = mINIParser.GetDoubleValue("FrameSettings", "UnfocusedFrameRate", defaults.UnfocusedFrameRate);
	mFrameSettings.UIFrameRate = mINIParser.GetDoubleValue("FrameSettings", "UIFrameRate", defaults.UIFrameRate);
}

void EngineConfig::LoadBackgroundTaskFilter()
{
	// Missing keys keep AsyncOperationFilter's defaults
	const AsyncOperationFilter defaults;
	mBackgroundTaskFilter.ShowSubSteps = mINIParser.GetBoolValue("BackgroundTasks", "ShowSubSteps", defaults.ShowSubSteps);
	mBackgroundTaskFilter.ShowBlocking = mINIParser.GetBoolValue("BackgroundTasks", "ShowBlocking", defaults.ShowBlocking);

	struct KindKey
	{
		const char* Key;
		EAsyncOperationKind Kind;
	};
	const KindKey kindKeys[] = {
		{ "ShowResourceLoads", EAsyncOperationKind::ResourceLoad },
		{ "ShowSceneLoads", EAsyncOperationKind::SceneLoad },
		{ "ShowRendering", EAsyncOperationKind::Rendering },
		{ "ShowSaving", EAsyncOperationKind::Saving },
		{ "ShowOther", EAsyncOperationKind::General }
	};
	for (const KindKey& kindKey : kindKeys)
	{
		mBackgroundTaskFilter.SetShowsKind(kindKey.Kind, mINIParser.GetBoolValue("BackgroundTasks", kindKey.Key, defaults.ShowsKind(kindKey.Kind)));
	}
}

void EngineConfig::LoadRenderingSettings()
{
	const char* const antiAliasingName = mINIParser.GetValue("Rendering", "AntiAliasing", GetAntiAliasingName(mAntiAliasing));
	if (const std::optional<EAntiAliasing> antiAliasing = ParseAntiAliasing(antiAliasingName))
	{
		mAntiAliasing = *antiAliasing;
	}
	else
	{
		RZE_LOG_WARNING_ARGS("Engine.ini's [Rendering] AntiAliasing '%s' isn't Off, Low, Medium or High; using %s.", antiAliasingName, GetAntiAliasingName(mAntiAliasing));
	}

	const char* const ambientOcclusionName = mINIParser.GetValue("Rendering", "AmbientOcclusion", GetAmbientOcclusionName(mAmbientOcclusion));
	if (const std::optional<EAmbientOcclusion> ambientOcclusion = ParseAmbientOcclusion(ambientOcclusionName))
	{
		mAmbientOcclusion = *ambientOcclusion;
	}
	else
	{
		RZE_LOG_WARNING_ARGS("Engine.ini's [Rendering] AmbientOcclusion '%s' isn't Off, Low, Medium or High; using %s.", ambientOcclusionName, GetAmbientOcclusionName(mAmbientOcclusion));
	}

	mAutoExposure = mINIParser.GetBoolValue("Rendering", "AutoExposure", mAutoExposure);

	mGlobalIllumination = mINIParser.GetBoolValue("Rendering", "GlobalIllumination", mGlobalIllumination);
	// At least 1 of each, so a misread value can't stop the probes from ever finishing
	GlobalIlluminationSettings& probes = mGlobalIlluminationSettings;
	probes.TargetProbeCount = static_cast<U32>(std::max(1L, mINIParser.GetLongValue("Rendering", "ProbeTargetCount", static_cast<long>(probes.TargetProbeCount))));
	probes.CaptureDrawBudget = static_cast<U32>(std::max(1L, mINIParser.GetLongValue("Rendering", "ProbeCaptureDrawBudget", static_cast<long>(probes.CaptureDrawBudget))));
	probes.RelightBudget = static_cast<U32>(std::max(1L, mINIParser.GetLongValue("Rendering", "ProbeRelightBudget", static_cast<long>(probes.RelightBudget))));
}

WindowSettings::WindowSettings()
{
}

WindowSettings::~WindowSettings()
{
}

const Vector2D& WindowSettings::GetDimensions() const
{
	return mDimensions;
}

const std::string& WindowSettings::GetTitle() const
{
	return mTitle;
}

int WindowSettings::GetLaunchMonitor() const
{
	return mLaunchMonitor;
}

EngineSettings::EngineSettings()
{
}

EngineSettings::~EngineSettings()
{
}

U32 EngineSettings::GetMSAASampleCount() const
{
	return mMSAASampleCount;
}
