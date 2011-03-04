// -*- C++ -*- 

#ifndef VIEWERWINDOW_H
#define VIEWERWINDOW_H

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
  wxTimer update_timer;
  wxRegion updated_area;

  void onUpdateTimer(wxTimerEvent& event);
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

  void adjustSize(); 
  const VNCConn* getConn() const { return conn; };
};
	



/*
  consists of two scrollable windows, one a container for a VNCCanvas 
  that centers the canvas in itself, the other one containing the stats window,
  also centered
*/
class ViewerWindow: public wxPanel
{
  wxScrolledWindow* canvas_container;
  wxScrolledWindow* stats_container;
  VNCCanvas* canvas;

  // timer to update stats win
  wxTimer stats_timer;
  void onStatsTimer(wxTimerEvent& event);

  // save default foreground colour to be able to flash when buffer full
  wxColour dflt_fg;

protected:
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
  ViewerWindow(wxWindow* parent);
  ~ViewerWindow();

  void setCanvas(VNCCanvas* c);
  wxScrolledWindow* getContainer() const { return canvas_container; };

  void showStats(bool yesno);
};
	

#endif
