#include <StdAfx.h>
#include <EngineCore/Config/EngineConfig.h>

#include <Utils/Conversions.h>

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
