#pragma once

#include <Utils/Functor.h>

// #BRIEF
// Base class for all render commands. Allows for interface for different render implementations.
// Necessary parameters are defined by derived classes.
class RenderCommandBase
{
public:
	RenderCommandBase();
	virtual ~RenderCommandBase();

	virtual void Execute() = 0;
};