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
