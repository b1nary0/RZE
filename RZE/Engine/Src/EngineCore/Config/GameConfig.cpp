#include <StdAfx.h>
#include <EngineCore/Config/GameConfig.h>

namespace
{
	constexpr const char* k_gameSection = "Game";
	constexpr const char* k_iconKey = "Icon";
}

GameConfig::GameConfig()
{
}

GameConfig::~GameConfig()
{
}

bool GameConfig::Load(const Filepath& filePath)
{
	mFilePath = filePath;
	if (filePath.Exists())
	{
		Config::Read(filePath);
	}

	const std::string icon = mINIParser.GetValue(k_gameSection, k_iconKey, "");
	mIcon = icon.empty() ? Filepath() : Filepath(icon);
	return true;
}

void GameConfig::Release()
{
}

const Filepath& GameConfig::GetIcon() const
{
	return mIcon;
}

void GameConfig::SetIcon(const Filepath& icon)
{
	mIcon = icon;
	mINIParser.SetValue(k_gameSection, k_iconKey, icon.IsValid() ? icon.GetRelativePath().c_str() : "");
}

bool GameConfig::Save() const
{
	return Config::Write(mFilePath);
}
