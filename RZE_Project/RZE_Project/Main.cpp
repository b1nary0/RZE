#include "StdAfx.h"

#include "Src/Application.h"

int main(void)
{
	RZE_Application app;
	app.Start();

	while (app.IsRunning())
	{
		app.Update();
	}

	getchar();
	return 0;
}