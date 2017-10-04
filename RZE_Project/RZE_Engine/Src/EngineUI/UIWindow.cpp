#include <StdAfx.h>

#include <EngineUI/UIWindow.h>

UIWindow::UIWindow(const std::string& title /*= ""*/)
	: mTitle(title)
{
}

UIWindow::~UIWindow()
{
}

void UIWindow::OnCreate()
{
}

void UIWindow::OnDraw()
{
}

void UIWindow::SetTitle(const std::string& title)
{
	mTitle = title;
}

const std::string& UIWindow::GetTitle()
{
	return mTitle;
}
