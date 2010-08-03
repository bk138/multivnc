// -*- C++ -*- 

#ifndef VNCCANVAS_H
#define VNCCANVAS_H

#include <wx/scrolwin.h>
#include <wx/dcclient.h>
#include <wx/log.h>
#include "VNCConn.h"



/*
  canvas that displays a VNCConn's framebuffer
  and submits mouse and key events.

*/
class VNCCanvas: public wxPanel
{
  VNCConn* conn;

  void onPaint(wxPaintEvent &event);
  void onMouseAction(wxMouseEvent &event);
  void onKeyDown(wxKeyEvent &event);
  void onKeyUp(wxKeyEvent &event);
  void onChar(wxKeyEvent &event);
  void onFocusLoss(wxFocusEvent &event);
  void onVNCConnUpdateNotify(VNCConnUpdateNotifyEvent& event);

protected:
  DECLARE_EVENT_TABLE();

public:
  VNCCanvas(wxWindow* parent, VNCConn* c);

  void drawRegion(wxRect& rect);
  void adjustSize(); 
};
	



/*
  a scrollable container for a VNCCanvas that centers the
  canvas in itself and takes care of deletion of its assigned 
  VNCCanvas
*/
class VNCCanvasContainer: public wxScrolledWindow
{
  VNCCanvas* canvas;

public:
  VNCCanvasContainer(wxWindow* parent);
  ~VNCCanvasContainer();

  void setCanvas(VNCCanvas* c);
  VNCCanvas* getCanvas() const;

};
	

#endif
