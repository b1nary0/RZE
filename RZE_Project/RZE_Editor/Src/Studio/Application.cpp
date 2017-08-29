#include <StdAfx.h>

#include <Studio/Application.h>

bool MyApp::OnInit()
{
	mFrame = std::make_unique<MyFrame>();

	mFrame->Maximize(true);
	mFrame->Show(true);

	return true;
}

int MyApp::OnExit()
{
	mFrame.release();

	return 0;
}

MyFrame& MyApp::GetFrame()
{
	AssertNotNull(mFrame);
	return *mFrame;
}

MyFrame::MyFrame()
	: wxFrame(NULL, wxID_ANY, "RZE Studio")
{
	wxMenu *menuFile = new wxMenu;
	menuFile->Append(ID_Hello, "&Hello...\tCtrl-H",
		"Help string shown in status bar for this menu item");
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT);

	wxMenu *menuHelp = new wxMenu;
	menuHelp->Append(wxID_ABOUT);

	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append(menuFile, "&File");
	menuBar->Append(menuHelp, "&Help");

	SetMenuBar(menuBar);
	CreateStatusBar();

	SetStatusText("Welcome to RZE Studio!");
	Bind(wxEVT_MENU, &MyFrame::OnHello, this, ID_Hello);
	Bind(wxEVT_MENU, &MyFrame::OnAbout, this, wxID_ABOUT);
	Bind(wxEVT_MENU, &MyFrame::OnExit, this, wxID_EXIT);
}

void MyFrame::OnExit(wxCommandEvent& event)
{
	Close(true);
}

void MyFrame::OnAbout(wxCommandEvent& event)
{
	wxMessageBox("This is a wxWidgets' Hello world sample",
		"About Hello World", wxOK | wxICON_INFORMATION);
}

void MyFrame::OnHello(wxCommandEvent& event)
{
	wxLogMessage("Hello world from wxWidgets!");
}