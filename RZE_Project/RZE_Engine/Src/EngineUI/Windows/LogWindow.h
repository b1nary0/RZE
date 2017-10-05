#pragma once

#include <vector>

#include <EngineUI/UIWindow.h>

#include <Utils/Math/Vector4D.h>

class LogWindow : public UIWindow
{
public:
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

private:
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
	Vector4D GetColorFromPriority(ELogPriorityType::T priority);

private:
	int mMaxItems;

	std::vector<LogInfoItem> mItems;
};