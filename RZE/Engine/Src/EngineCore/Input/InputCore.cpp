#include <StdAfx.h>

#include <EngineCore/Input/InputCore.h>

/*
 * InputKey
 */

InputKey::InputKey(const std::string& displayName, Int32 keyCode)
	: mDisplayName(displayName)
	, mKeyCode(keyCode)
{
}
