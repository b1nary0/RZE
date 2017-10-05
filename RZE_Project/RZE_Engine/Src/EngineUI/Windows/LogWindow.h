#pragma once

#include <vector>

#include <EngineUI/UIWindow.h>

class LogWindow : public UIWindow
{
	struct ELogPriorityType
	{
		enum T
		{
			Debug,
			Info,
			Warning,
			Error
		};
	};

	struct LogInfoItem
	{
		ELogPriorityType::T mPriority;
		std::string mBody;
	};

public:
	LogWindow();
	~LogWindow();

	virtual void OnCreate() override;
	virtual void OnDraw() override;

	void SetMaxItemCount(int newMaxItemCount);
	int GetMaxItemCount();

	void Add(const std::string& text, ELogPriorityType::T priority = ELogPriorityType::Info);

private:
	int mMaxItems;

	std::vector<LogInfoItem> mItems;
};