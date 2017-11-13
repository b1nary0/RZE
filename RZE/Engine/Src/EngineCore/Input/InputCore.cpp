#include <StdAfx.h>

#include <EngineCore/Input/InputCore.h>

/*
 * InputKey
 */

InputKey::InputKey(const char displayName, Int32 keyCode)
	: mDisplayName(displayName)
	, mKeyCode(keyCode)
{
}
