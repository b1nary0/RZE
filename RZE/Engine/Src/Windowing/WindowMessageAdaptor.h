#pragma once

#include <queue>

class WindowMessageAdaptor
{
public:

	struct EMessageType
	{
		enum Value
		{
			Window_Create,
			Window_Move,
			Window_Resize,
			Window_Destroy,
			Window_Close,
			Window_Quit,
		};
	};

	struct WindowMessageInfo
	{
		EMessageType::Value		mMessageType;
		U64					wParam;
		Int64					lParam;
	};

	WindowMessageAdaptor();

	WindowMessageInfo GetNextMessage();
	void PushMessage(const EMessageType::Value messageType, U64 wParam, Int64 lParam);

	bool HasMessage();

private:

	std::queue<WindowMessageInfo> mMessageQueue;
};
