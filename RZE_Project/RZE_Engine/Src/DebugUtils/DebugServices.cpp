#include <StdAfx.h>

#include <DebugUtils/DebugServices.h>

DebugServices* DebugServices::sInstance = nullptr;

DebugServices::DebugServices()
{
	const Vector2D& screenSize = RZE_Engine::Get()->GetWindowSettings().GetDimensions();

	mLogWindow.SetSize(Vector2D(screenSize.X() * 0.5f, screenSize.Y() * 0.25f));
	mLogWindow.SetPosition(Vector2D(10.0f, screenSize.Y() - (mLogWindow.GetSize().Y() + 10)));
}

DebugServices::~DebugServices()
{

}

void DebugServices::LogDebug(const std::string& msg)
{
	mLogWindow.Add(msg, LogWindow::ELogPriorityType::Debug);
}

void DebugServices::LogInfo(const std::string& msg)
{
	mLogWindow.Add(msg, LogWindow::ELogPriorityType::Info);
}

void DebugServices::LogWarning(const std::string& msg)
{
	mLogWindow.Add(msg, LogWindow::ELogPriorityType::Warning);
}

void DebugServices::LogError(const std::string& msg)
{
	mLogWindow.Add(msg, LogWindow::ELogPriorityType::Error);
}
