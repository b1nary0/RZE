#pragma once

// What the OpenGL backend's files share: the GL functions (glad, loaded by GLDevice) and the log for what isn't written yet

#include <glad/gl.h>

#include <Utils/DebugUtils/Debug.h>

// For a part of the OpenGL backend that does nothing yet: says so in the log, once per place, and carries on
#define RZE_GL_NOT_IMPLEMENTED()																	\
	do {																							\
		static bool s_hasLogged = false;															\
		if (!s_hasLogged)																			\
		{																							\
			s_hasLogged = true;																		\
			RZE_LOG_WARNING("OpenGL: " << __func__ << " isn't implemented yet; it does nothing.");	\
		}																							\
	} while (false)
