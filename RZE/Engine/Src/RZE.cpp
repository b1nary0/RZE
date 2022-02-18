#include <StdAfx.h>

#include <RZE.h>

#include <EngineApp.h>

namespace RZE
{
	// #NOTE
	// The functions here that reach into RZE_Application to get at the Engine instance
	// is an example of why this pattern is shite. The idea was to give an API within the app itself,
	// but in use, things like this are annoying; because it is only here to make the call with more
	// syntactical brevity.
	ResourceHandler& GetResourceHandler()
	{
		return RZE_Application::RZE().GetResourceHandler();
	}

	RenderEngine& GetRenderEngine()
	{
		return RZE_Application::RZE().GetRenderEngine();
	}

	GameScene& GetActiveScene()
	{
		return RZE_Application::RZE().GetActiveScene();
	}
}