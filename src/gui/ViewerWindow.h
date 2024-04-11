// -*- C++ -*- 

#ifndef VIEWERWINDOW_H
#define VIEWERWINDOW_H

#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <wx/log.h>
#include "VNCConn.h"

class VNCCanvas;

/*
  consists of two scrollable windows, one a container for a VNCCanvas 
  that centers the canvas in itself, the other one containing the stats window,
  also centered
*/
class ViewerWindow: public wxPanel
{
public:
  ViewerWindow(wxWindow* parent, VNCConn* connection);
  ~ViewerWindow();

  void adjustCanvasSize(); 

  void showStats(bool yesno);
  void showOneToOne(bool show_1to1);
  void grabKeyboard(bool yesno);


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

private:

  wxScrolledWindow* canvas_container;
  wxScrolledWindow* stats_container;
  VNCCanvas* canvas;

  // timer to update stats win
  wxTimer stats_timer;
  void onStatsTimer(wxTimerEvent& event);
  void onResize(wxSizeEvent &event);
  void onVNCConnUpdateNotify(VNCConnUpdateNotifyEvent& event);

  // save default foreground colour to be able to flash when buffer full
  wxColour dflt_fg;

  bool show_1to1;

};
	

#endif
