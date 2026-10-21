#pragma once

#include <EngineCore/Config/Config.h>

#include <Utils/Platform/Filepath.h>

// Config/Game.ini: what the game itself is, as opposed to how the engine runs it (Engine.ini). The editor's Project
// Settings change it and save it; the game reads it at startup.
class GameConfig final : public Config
{
public:
	GameConfig();
	~GameConfig() override;

	// A missing file leaves every setting at its default, and Save creates it
	bool Load(const Filepath& filePath) override;
	void Release() override;

	// [Game] Icon: the image the game's window shows; invalid for the executable's own icon
	const Filepath& GetIcon() const;
	void SetIcon(const Filepath& icon);

	// Writes the settings over the file they were loaded from. False if it couldn't.
	bool Save() const;

private:
	Filepath mFilePath;
	Filepath mIcon;
};
