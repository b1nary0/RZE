#pragma once
#pragma message("*********** Compiling Engine PCH... ***********")

// WinAPI
#include <Windows.h>
#include <Windowsx.h>

// std lib
#include <assert.h>
#include <codecvt>
#include <cstdint>
#include <locale.h>
#include <string>
#include <math.h>
#include <limits>

// std io
#include <cstdio>
#include <iostream>

// std memory
#include <memory>

// std containers
#include <map>
#include <queue>
#include <vector>

// Optick
#include <Optick/optick.h>

// RZE
#include <RZE_Config.h>
#include <RZE.h>
#include <EngineApp.h>
#include <Utils/DebugUtils/Debug.h>
#include <Utils/Platform/FilePath.h>
#include <Utils/PrimitiveDefs.h>

#ifdef DEBUG

#include <DebugUtils/DebugServices.h>

#endif

#include <imGUI/imgui.h>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif
