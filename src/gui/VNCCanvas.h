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
  const VNCConn* getConn() const { return conn; };
};
	



/*
  a scrollable container for a VNCCanvas that centers the
  canvas in itself and takes care of deletion of its assigned 
  VNCCanvas
*/
class VNCCanvasContainer: public wxScrolledWindow
{
  VNCCanvas* canvas;

  // timer to update stats win
  wxTimer stats_timer;
  void onStatsTimer(wxTimerEvent& event);

  // save default foreground colour to be able to flash when buffer full
  wxColour dflt_fg;

protected:
  wxStaticBox* sizer_stats_staticbox;
  wxStaticText* label_updrawbytes;
  wxTextCtrl* text_ctrl_updrawbytes;
  wxStaticText* label_updcount;
  wxTextCtrl* text_ctrl_updcount;
  wxStaticText* label_latency;
  wxTextCtrl* text_ctrl_latency;
  wxStaticText* label_lossratio;
  wxTextCtrl* text_ctrl_lossratio;
  wxStaticText* label_recvbuf;
  wxGauge* gauge_recvbuf;

  DECLARE_EVENT_TABLE();

public:
  VNCCanvasContainer(wxWindow* parent);
  ~VNCCanvasContainer();

  void setCanvas(VNCCanvas* c);
  VNCCanvas* getCanvas() const;

  void showStats(bool yesno);
};
	

#endif
