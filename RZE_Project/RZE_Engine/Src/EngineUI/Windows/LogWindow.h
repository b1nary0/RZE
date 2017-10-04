#pragma once

#include <vector>

#include <EngineUI/UIWindow.h>

class LogWindow : public UIWindow
{
public:
	LogWindow();
	~LogWindow();

	virtual void OnCreate() override;
	virtual void OnDraw() override;

	void SetMaxItemCount(int newMaxItemCount);
	int GetMaxItemCount();

	void Add(const std::string& item);

private:
	int mMaxItems;

	std::vector<std::string> mItems;
};