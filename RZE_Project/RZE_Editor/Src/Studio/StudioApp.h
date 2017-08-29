#pragma once

// wxWidgets "Hello world" Program
// For compilers that support precompilation, includes "wx/wx.h".
#pragma warning(push, 0)
#include <wx/wxprec.h>
#include <wx/glcanvas.h>
#pragma warning(pop)

#ifndef WX_PRECOMP

#include <WX/wx.h>

#endif

#include <memory>

#include <EngineApp.h>

enum
{
	ID_Hello = 1
};

class MyFrame : public wxFrame
{
public:
	MyFrame();

private:
	void OnHello(wxCommandEvent& event);
	void OnExit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
};

class MyApp : public wxApp
{
public:
	virtual bool OnInit();
	virtual int OnExit();

	MyFrame& GetFrame();

private:
	std::unique_ptr<RZE_Game> mEngineBridge;

	std::unique_ptr<MyFrame> mFrame;
};
