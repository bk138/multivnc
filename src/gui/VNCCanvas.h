// -*- C++ -*- 

#ifndef VNCCANVAS_H
#define VNCCANVAS_H

#include <wx/scrolwin.h>
#include <wx/dcclient.h>
#include <wx/log.h>
#include "VNCConn.h"

	
/*
  a scrollable canvas that displays a VNCConn's framebuffer
  and submits mouse and key events.

*/

class VNCCanvas: public wxScrolledWindow
{
  VNCConn* conn;
  wxRect framebuffer_rect;


  void onPaint(wxPaintEvent &event);
  void onMouseAction(wxMouseEvent &event);
  void onKeyDown(wxKeyEvent &event);
  void onKeyUp(wxKeyEvent &event);
  void onChar(wxKeyEvent &event);


protected:
  DECLARE_EVENT_TABLE();

public:
  VNCCanvas(wxWindow* parent, VNCConn* c);

  void drawRegion(wxRect& rect);

};
	

#endif
