#pragma once

// Which debugging aids a build has. Debug and Development have them all; Retail, the build that ships, strips them.
// Small, so a header that only needs to know (LiveObjectPtr.h) can include it without Debug.h.

// Asserts (Debug.h) and the checks that exist only to feed them
#ifndef RZE_RETAIL
#define RZE_ASSERTS_ENABLED
#endif

// The RZE_LOG macros (Debug.h)
#ifndef RZE_RETAIL
#define RZE_LOGGING_ENABLED
#endif

// The Game's ImGui: its render stage, input and demo window (GameApp.cpp). The Editor's is always on.
#ifndef RZE_RETAIL
#define IMGUI_ENABLED
#endif

// RZE_DISABLE_OPTIMIZATION (Debug.h). Without it, the macro is a compile error, so one left in code can't ship.
#ifndef RZE_RETAIL
#define RZE_OPTIMIZATION_SWITCH_ENABLED
#endif
