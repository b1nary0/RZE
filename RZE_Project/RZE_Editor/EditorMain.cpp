#include <Studio/StudioApp.h>

// 
// class wxGLCanvasSubClass : public wxGLCanvas {
// 	void Render();
// public:
// 	wxGLCanvasSubClass(wxFrame* parent);
// 	void Paintit(wxPaintEvent& event);
// protected:
// 	DECLARE_EVENT_TABLE()
// };
// 
// BEGIN_EVENT_TABLE(wxGLCanvasSubClass, wxGLCanvas)
// EVT_PAINT(wxGLCanvasSubClass::Paintit)
// END_EVENT_TABLE()
// 
// wxGLCanvasSubClass::wxGLCanvasSubClass(wxFrame *parent)
// 	:wxGLCanvas(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, wxT("GLCanvas")) {
// 	int argc = 1;
// 	char* argv[1] = { wxString((wxTheApp->argv)[0]).char_str() };
// 
// 	/*
// 	NOTE: this example uses GLUT in order to have a free teapot model
// 	to display, to show 3D capabilities. GLUT, however, seems to cause problems
// 	on some systems. If you meet problems, first try commenting out glutInit(),
// 	then try comeenting out all glut code
// 	*/
// }
// 
// void wxGLCanvasSubClass::Paintit(wxPaintEvent& WXUNUSED(event)) {
// 	Render();
// }

wxIMPLEMENT_APP(MyApp);
