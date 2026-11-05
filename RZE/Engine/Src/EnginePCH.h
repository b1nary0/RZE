#pragma once

// The std, Utils and maths layers every project shares
#include <UtilsPCH.h>

// std, beyond UtilsPCH.h's
#include <cstdio>
#include <iostream>
#include <limits>
#include <queue>

// Optick
#include <Optick/optick.h>

// RZE
#include <RZE_Config.h>
#include <RZE.h>
#include <EngineApp.h>

// IMGUI
#include <imgui/imgui.h>

#ifdef RZE_DEBUG

#include <DebugUtils/DebugServices.h>

#endif
