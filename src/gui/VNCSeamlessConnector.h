// -*- C++ -*- 

#ifndef VNCSEAMLESSCONNECTOR_H
#define VNCSEAMLESSCONNECTOR_H


#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "wx/frame.h"
#include <wx/timer.h>
#include "VNCConn.h"

#define EDGE_EAST 0
#define EDGE_WEST 1
#define EDGE_NORTH 2 
#define EDGE_SOUTH 3


/*
  VNCSeamlessConnector basically is a modified x2vnc to
  blend in with MultiVNC: this creates a small window on some
  screen edge and submits mouse and key events.

*/
class VNCSeamlessConnector: public wxFrame
{
public:
  VNCSeamlessConnector(wxWindow* parent, VNCConn* c, int e);
  ~VNCSeamlessConnector();

  void adjustSize(); 



protected:
  DECLARE_EVENT_TABLE();

private:
  VNCConn* conn;
  wxSize framebuffer_size;
  wxSize display_size;
  int edge;

  void OnMouse(wxMouseEvent& evt);

  wxString err;

  wxTimer runtimer;

  Display *dpy;

  void onRuntimer(wxTimerEvent& event);
  

  Bool CreateXWindow();
  Bool HandleXEvents();
  void doWarp();
  int enter_translate(int isedge, int width, int pos);
  int leave_translate(int isedge, int width, int pos);
  int sendpointerevent(int x, int y, int buttonmask);
  void mapwindow(void);
  void hidewindow(void);
  void grabit(int x, int y, int state);
  void ungrabit(int x, int y, Window warpWindow);
  
  void dumpMotionEvent(XEvent *ev);
  int coord_dist_sq(wxPoint a, wxPoint b);
  int coord_dist_from_edge(wxPoint a);
  int get_root_int_prop(Atom property);
  void check_desktop(void);
  void handle_cut_text(char *str, size_t len);
  Bool HandleTopLevelEvent(XEvent *ev);
  Bool HandleRootEvent(XEvent *ev); 



#ifdef HAVE_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#ifdef HAVE_XRANDR
#include <X11/extensions/Xrandr.h>

  int server_has_randr=0;
  int xrandr_event_base;
  int xrandr_error_base;
#endif


  float acceleration;
  Bool resurface;
  

  Window topLevel;


  Atom wmProtocols, wmDeleteWindow, wmState;
  Bool modifierPressed[256];

  int x_offset, y_offset;
  int grabbed;
  int pointer_warp_threshold;
  Cursor  grabCursor;

  float remote_xpos;
  float remote_ypos;
  float pointer_speed;

  wxPoint origo1, origo2;
  wxPoint current_location;
  wxPoint current_speed;
  wxPoint * next_origo;
  wxPoint * current_origo;
  int origo_separation;
  int motion_events;

#define XROOT(E) ((E).x_root - x_offset)
#define YROOT(E) ((E).y_root - y_offset)

  int edge_width;
  int restingx;
  int restingy;
  int hidden;
  int debug;

  char *client_selection_text;
  size_t client_selection_text_length;

  int saved_xpos;
  int saved_ypos;
  int saved_remote_xpos;
  int saved_remote_ypos;

  /* We must do our own desktop handling */
  Atom current_desktop_atom;
  Atom number_of_desktops_atom;
  int requested_desktop;
  int current_desktop;


  /*
   * This variable is true (1) if the mouse is on the same screen as the one
   * we're monitoring, or if there is only one screen on the X server.
   * - GRM
   */
  Bool mouseOnScreen;
};


#endif //VNCSEAMLESSCONNECTOR_H
