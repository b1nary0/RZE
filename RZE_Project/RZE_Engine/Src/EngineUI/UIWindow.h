#pragma once

#include <string>

#include <EngineUI/UIElement.h>

class UIWindow : public UIElement
{
public:
	UIWindow(const std::string& title = "");
	virtual ~UIWindow();

	virtual void OnCreate() override;
	virtual void OnDraw() override;

	void SetTitle(const std::string& title);
	const std::string& GetTitle();

protected:

private:
	std::string mTitle;
};
