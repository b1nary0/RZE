#pragma once

#include <EngineCore/Config/Config.h>

#include <Utils/Math/Vector2D.h>

class FilePath;

class WindowSettings
{
	friend class EngineConfig;

public:
	WindowSettings();
	~WindowSettings();

	const Vector2D& GetDimensions() const;
	const std::string& GetTitle() const;

private:
	std::string mTitle;
	Vector2D mDimensions;
};

class EngineConfig : public Config
{
public:
	EngineConfig();
	~EngineConfig();

	virtual void Load(const FilePath& filePath) override;

	WindowSettings& GetWindowSettings();

private:
	void LoadWindowSettings();

private:
	WindowSettings mWindowSettings;
};
