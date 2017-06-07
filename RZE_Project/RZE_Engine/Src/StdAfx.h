#pragma once
#pragma message("*********** Compiling PCH... ***********")

// WinAPI
#include <Windows.h>

#define GLEW_STATIC
#include <GL/glew.h>
#include <gl/GLU.h>

// std lib
#include <assert.h>
#include <codecvt>
#include <cstdint>
#include <locale.h>
#include <string>

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

// RZE
#include "RZE_Config.h"
#include "RZE.h"
#include "DebugUtils/Debug.h"
#include "Utils/PrimitiveDefs.h"