#pragma once
#pragma message("*********** Compiling PCH... ***********")

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

// glm
 #include <GLM/glm.hpp>
 #include <GLM/common.hpp>
 #include <GLM/gtc/matrix_transform.hpp>
 #include <GLM/gtc/type_ptr.hpp>

// RZE
#include "RZE_Config.h"
#include "RZE.h"
#include "DebugUtils/Debug.h"
#include "Utils/PrimitiveDefs.h"