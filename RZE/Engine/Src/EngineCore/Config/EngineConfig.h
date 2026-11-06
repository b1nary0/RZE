#pragma once

#include <EngineCore/Async/AsyncOperationFilter.h>
#include <EngineCore/Config/Config.h>
#include <EngineCore/Config/FrameSettings.h>

#include <Graphics/AmbientOcclusion.h>
#include <Graphics/AntiAliasing.h>
#include <Graphics/GlobalIllumination.h>

#include <Utils/PrimitiveDefs.h>
#include <Utils/Math/Vector2D.h>

class EnumNames;
class Filepath;

class EngineSettings
{
	friend class EngineConfig;

public:
	EngineSettings();
	~EngineSettings();

public:
	U32 GetMSAASampleCount() const;

private:
	U32 mMSAASampleCount;
};

class WindowSettings
{
	friend class EngineConfig;

public:
	WindowSettings();
	~WindowSettings();

	const Vector2D& GetDimensions() const;
	const std::string& GetTitle() const;
	int GetLaunchMonitor() const;

private:
	std::string mTitle;
	Vector2D mDimensions;
	int mLaunchMonitor{ 0 };
};

class EngineConfig final : public Config
{
public:
	EngineConfig();
	~EngineConfig() override;

	virtual bool Load(const Filepath& filePath) override;
	virtual void Release() override;

	const EngineSettings& GetEngineSettings() const;
	const WindowSettings& GetWindowSettings() const;
	const FrameSettings& GetFrameSettings() const;
	// [BackgroundTasks]: which operations progress displays show
	const AsyncOperationFilter& GetBackgroundTaskFilter() const;
	// [Rendering]: the main view's anti-aliasing, ambient occlusion and auto-exposure at start-up
	EAntiAliasing GetAntiAliasing() const { return mAntiAliasing; }
	EAmbientOcclusion GetAmbientOcclusion() const { return mAmbientOcclusion; }
	bool GetAutoExposure() const { return mAutoExposure; }
	// [Rendering]: whether the main view uses the irradiance probes, and how much work they get
	bool GetGlobalIllumination() const { return mGlobalIllumination; }
	const GlobalIlluminationSettings& GetGlobalIlluminationSettings() const { return mGlobalIlluminationSettings; }

private:
	void LoadEngineSettings();
	void LoadWindowSettings();
	void LoadFrameSettings();
	void LoadBackgroundTaskFilter();
	void LoadRenderingSettings();

	// [section] key as one of names (ignoring case), or current if it's missing; warns and returns current if it isn't
	// one of them
	size_t ReadEnumSetting(const char* section, const char* key, const EnumNames& names, size_t current) const;

private:
	EngineSettings mEngineSettings;
	WindowSettings mWindowSettings;
	FrameSettings mFrameSettings;
	AsyncOperationFilter mBackgroundTaskFilter;
	EAntiAliasing mAntiAliasing = EAntiAliasing::Medium;
	EAmbientOcclusion mAmbientOcclusion = EAmbientOcclusion::Medium;
	bool mAutoExposure = true;
	bool mGlobalIllumination = true;
	GlobalIlluminationSettings mGlobalIlluminationSettings;
};
