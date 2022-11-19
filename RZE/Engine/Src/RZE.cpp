#include <StdAfx.h>

#include <RZE.h>

namespace
{
	RZE_Engine g_engine;
}

RZE_Engine& RZE()
{
	return g_engine;
}