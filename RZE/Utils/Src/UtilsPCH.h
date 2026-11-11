#pragma once

// Utils' precompiled header, and the base of every other project's (EnginePCH.h includes it first): what's costly to
// parse and seldom changes. An edit to a header here rebuilds every project that uses it, so project headers that
// change often belong in the projects' own sources, not here.

// std lib
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <math.h>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Utils core
#include <Utils/DebugUtils/Debug.h>
#include <Utils/Platform/File.h>
#include <Utils/Platform/Filepath.h>
#include <Utils/PrimitiveDefs.h>

// Utils' maths, and through it GLM, which only Utils/Math includes
#include <Utils/Math/BoundingBox.h>
#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Quaternion.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>
