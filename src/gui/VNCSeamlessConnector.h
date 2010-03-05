// -*- C++ -*- 

#ifndef VNCSEAMLESSCONNECTOR_H
#define VNCSEAMLESSCONNECTOR_H

/*
#include <X11/Xlib.h>
#include <X11/Xatom.h>
*/

#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/timer.h>
#include "VNCConn.h"

#define EDGE_NONE -1
#define EDGE_EAST  0
#define EDGE_WEST  1
#define EDGE_NORTH 2 
#define EDGE_SOUTH 3



class VNCSeamlessConnectorCanvas;


/*
  VNCSeamlessConnector basically is a modified x2vnc to
  blend in with MultiVNC: this creates a small window on some
  screen edge and submits mouse and key events.

*/
class VNCSeamlessConnector: public wxFrame
{
public:
  VNCSeamlessConnector(wxWindow* parent, VNCConn* c, int edge, size_t edge_width=5, float acceleration=1.0);
  ~VNCSeamlessConnector();

  void adjustSize(); 



protected:
  // DECLARE_EVENT_TABLE();

private:
  VNCConn* conn;

  wxSize framebuffer_size;
  wxSize display_size;
  size_t display_index;

  int edge;
  int edge_width;

  wxPoint multiscreen_offset;

  bool grabbed;
  
  wxPoint origo1, origo2;
  wxPoint * next_origo;
  wxPoint * current_origo;
  int origo_separation;

  float acceleration;
  float pointer_speed;

  int pointer_warp_threshold;
  wxPoint current_location;
  wxPoint current_speed;

  wxRealPoint remotePos;
  wxPoint remoteParkingPos;

  void doWarp();
  int enter_translate(int isedge, int width, int pos);
  int leave_translate(int isedge, int width, int pos);
  void grabit(int x, int y, int state);
  void ungrabit(int x, int y);
  int coord_dist_sq(wxPoint a, wxPoint b);
  int coord_dist_from_edge(wxPoint a);

  //this is needed as GTK does not give key events for toplevel windows
  friend class VNCSeamlessConnectorCanvas;
  VNCSeamlessConnectorCanvas* canvas; 

  // these are called by the canvas
  void handleMouse(wxMouseEvent& evt);
  void handleKey(wxKeyEvent& evt);
  void handleFocusLoss();
  void handleCaptureLoss();





  // x2vnc stuff
  int motion_events;

  /*
  wxTimer runtimer;
  Display *dpy;
#ifdef HAVE_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif
#ifdef HAVE_XRANDR
#include <X11/extensions/Xrandr.h>
  int server_has_randr=0;
  int xrandr_event_base;
  int xrandr_error_base;
#endif
  Bool resurface;
  Window topLevel;
  Atom wmProtocols, wmDeleteWindow, wmState;
  Bool modifierPressed[256];
  Cursor  grabCursor;

  int hidden;
  int debug;

  char *client_selection_text;
  size_t client_selection_text_length;

  // seems to have to do with hotkey warping
  int saved_xpos;
  int saved_ypos;
  int saved_remote_xpos;
  int saved_remote_ypos;

 
 //   This variable is true (1) if the mouse is on the same screen as the one
  //  we're monitoring, or if there is only one screen on the X server.
  //  - GRM
   
  Bool mouseOnScreen;

  void onRuntimer(wxTimerEvent& event);
  Bool CreateXWindow();
  Bool HandleXEvents();
  int sendpointerevent(int x, int y, int buttonmask);
  void mapwindow(void);
  void hidewindow(void);
  void dumpMotionEvent(XEvent *ev);
  void handle_cut_text(char *str, size_t len);
  Bool HandleTopLevelEvent(XEvent *ev);
  Bool HandleRootEvent(XEvent *ev); 
*/
};





/*
  this is needed as GTK does not give key events for toplevel windows,
  it just passes events it receives to its parent
*/
class VNCSeamlessConnectorCanvas: public wxPanel
{
public:
  VNCSeamlessConnectorCanvas(VNCSeamlessConnector* parent);

protected:
  DECLARE_EVENT_TABLE();

private:
  VNCSeamlessConnector* p;

  // event handlers
  void onMouse(wxMouseEvent &event);
  void onKeyDown(wxKeyEvent &event);
  void onKeyUp(wxKeyEvent &event);
  void onChar(wxKeyEvent &event);
  void onFocusLoss(wxFocusEvent &event);
  void onCaptureLoss(wxMouseCaptureLostEvent &event);
};


#endif //VNCSEAMLESSCONNECTOR_H
