#pragma once

// Which debugging aids a build has. Debug and Development have them all; Retail, the build that ships, strips them.
// Small, so a header that only needs to know (LiveObjectPtr.h) can include it without Debug.h.

// A development build: Debug and Development. Every switch below comes from it, so this is the one place that knows
// which configs are development ones. Code that exists only for development as a whole (a test component, say) tests
// RZE_DEVEL; an aid with a switch of its own tests that switch, so the aid can be turned on or off by itself.
#ifndef RZE_RETAIL
#define RZE_DEVEL
#endif

// Asserts (Debug.h) and the checks that exist only to feed them
#ifdef RZE_DEVEL
#define RZE_ASSERTS_ENABLED
#endif

// The RZE_LOG macros (Debug.h)
#ifdef RZE_DEVEL
#define RZE_LOGGING_ENABLED
#endif

// The Game's ImGui: its render stage, input and demo window (GameApp.cpp). The Editor's is always on.
#ifdef RZE_DEVEL
#define IMGUI_ENABLED
#endif

// The graphics API's validation (the D3D11 debug layer, an OpenGL debug context), which reports to the log and asserts
// on errors (Engine.cpp's GetGraphicsDeviceOptions). It costs CPU time in every API call, so only Debug has it on
// unless -gfxvalidation says otherwise; in Development it's -gfxvalidation away. -gfxliveobjects, which logs the
// graphics objects still alive at shutdown, turns it on too. Retail has nothing to report to.
#ifdef RZE_DEVEL
#define RZE_GFX_VALIDATION_AVAILABLE
#endif
#ifdef _DEBUG
#define RZE_GFX_VALIDATION_BY_DEFAULT
#endif

// The debug-line render stage (RenderEngine.cpp), which draws DrawLine, DrawWireBox and DrawWireSphere
#ifdef RZE_DEVEL
#define RZE_DEBUG_DRAW_ENABLED
#endif

// RZE_DISABLE_OPTIMIZATION (Debug.h). Without it, the macro is a compile error, so one left in code can't ship.
#ifdef RZE_DEVEL
#define RZE_OPTIMIZATION_SWITCH_ENABLED
#endif
