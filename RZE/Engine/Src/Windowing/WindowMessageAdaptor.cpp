#include "StdAfx.h"

#include "WindowMessageAdaptor.h"

WindowMessageAdaptor::WindowMessageAdaptor()
{

}

WindowMessageAdaptor::WindowMessageInfo WindowMessageAdaptor::GetNextMessage()
{
	WindowMessageInfo info = mMessageQueue.front();
	mMessageQueue.pop();

	return info;
}

void WindowMessageAdaptor::PushMessage(const EMessageType::Value messageType, U64 wParam, Int64 lParam)
{
	WindowMessageInfo info;
	info.mMessageType = messageType;
	info.wParam = wParam;
	info.lParam = lParam;

	mMessageQueue.push(info);
}

bool WindowMessageAdaptor::HasMessage()
{
	return !mMessageQueue.empty();
}
