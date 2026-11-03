#include <StdAfx.h>

#include <Windowing/SplashScreen.h>

// Shows nothing yet on Linux: start-up runs as it does on Windows, without the splash screen over it

struct SplashScreen::State
{
};

SplashScreen::SplashScreen(const Filepath& /*image*/, const std::string& /*title*/, int /*monitorIndex*/)
	: m_state(std::make_unique<State>())
{
}

SplashScreen::~SplashScreen()
{
}

void SplashScreen::SetStatus(const std::string& /*status*/)
{
}

void SplashScreen::SetStatus(const std::string& /*status*/, float /*progress*/)
{
}
