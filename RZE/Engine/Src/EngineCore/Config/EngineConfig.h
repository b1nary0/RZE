#pragma once

#include <EngineCore/Async/AsyncOperationFilter.h>
#include <EngineCore/Config/Config.h>
#include <EngineCore/Config/FrameSettings.h>

#include <Graphics/AntiAliasing.h>

#include <Utils/PrimitiveDefs.h>
#include <Utils/Math/Vector2D.h>

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
	// [Rendering]: the main view's anti-aliasing and auto-exposure at start-up
	EAntiAliasing GetAntiAliasing() const { return mAntiAliasing; }
	bool GetAutoExposure() const { return mAutoExposure; }

private:
	void LoadEngineSettings();
	void LoadWindowSettings();
	void LoadFrameSettings();
	void LoadBackgroundTaskFilter();
	void LoadRenderingSettings();

private:
	EngineSettings mEngineSettings;
	WindowSettings mWindowSettings;
	FrameSettings mFrameSettings;
	AsyncOperationFilter mBackgroundTaskFilter;
	EAntiAliasing mAntiAliasing = EAntiAliasing::Medium;
	bool mAutoExposure = true;
};
