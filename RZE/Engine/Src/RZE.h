#pragma once

#define STRINGIFY(Arg) #Arg

#include <EngineCore/Engine.h>

class RenderEngine;

namespace RZE
{
	RenderEngine& GetRenderEngine();
	ResourceHandler& GetResourceHandler();
}
