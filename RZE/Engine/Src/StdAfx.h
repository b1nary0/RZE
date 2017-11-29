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

// std io
#include <cstdio>
#include <iostream>

// std memory
#include <memory>

// std containers
#include <map>
#include <queue>
#include <vector>

// RZE
#include "RZE_Config.h"
#include "RZE.h"
#include "Utils/DebugUtils/Debug.h"
#include "Utils/PrimitiveDefs.h"