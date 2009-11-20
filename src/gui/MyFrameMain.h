// -*- C++ -*- 

#ifndef MYFRAMEMAIN_H
#define MYFRAMEMAIN_H


#include <vector>
#include <set>
#include "FrameMain.h"
#include "MyFrameLog.h"
#include "wxServDisc/wxServDisc.h"
#include "VNCConn.h"




class MyFrameMain: public FrameMain
{
  // main service scanner
  wxServDisc* servscan;

  // array of connections
  std::vector<VNCConn*> connections;
  // set of reverse VNC (listen) connection's port numbers
  std::set<int> listen_ports;

  // listbox_services_select() stores values here for listbox_services_dclick()
  wxString services_hostname, services_addr, services_port;

  // gui layout stuff
  bool show_toolbar;
  bool show_discovered;
  bool show_bookmarks;
  bool show_stats;
  bool show_fullscreen;

  // log window
  MyFrameLog* logwindow;

  // timer to update stats win
  wxTimer stats_timer;
  void onStatsTimer(wxTimerEvent& event);

  void splitwinlayout();

  bool spawn_conn(bool listen, wxString hostname, wxString addr, wxString port);
  void terminate_conn(int which);
  
  // private handlers
  void onMyFrameLogCloseNotify(wxCommandEvent& event);
  void onVNCConnUpdateNotify(wxCommandEvent& event);
  void onVNCConnFBResizeNotify(wxCommandEvent& event);
  void onVNCConnDisconnectNotify(wxCommandEvent& event);
  void onVNCConnIncomingConnectionNotify(wxCommandEvent& event);
  void onVNCConnCuttextNotify(wxCommandEvent& event);
  void onSDNotify(wxCommandEvent& event);


  
  static char* getpasswd(rfbClient* client);

  bool saveArrayString(wxArrayString& arrstr, wxString& path);
 
  
protected:
  DECLARE_EVENT_TABLE();
 
public:
  MyFrameMain(wxWindow* parent, int id, const wxString& title, 
	      const wxPoint& pos=wxDefaultPosition, 
	      const wxSize& size=wxDefaultSize, 
	      long style=wxDEFAULT_FRAME_STYLE);
  ~MyFrameMain();

  
  // handlers
  void listbox_services_select(wxCommandEvent &event); 
  void listbox_services_dclick(wxCommandEvent &event); 
  void listbox_bookmarks_select(wxCommandEvent &event); 
  void listbox_bookmarks_dclick(wxCommandEvent &event); 

  void machine_connect(wxCommandEvent &event);
  void machine_listen(wxCommandEvent &event);
  void machine_disconnect(wxCommandEvent &event);
  void machine_preferences(wxCommandEvent &event);
  void machine_showlog(wxCommandEvent &event);
  void machine_screenshot(wxCommandEvent &event);
  void machine_save_stats_upd(wxCommandEvent &event); 
  void machine_save_stats_lat(wxCommandEvent &event); 
  void machine_exit(wxCommandEvent &event);

  void view_toggletoolbar(wxCommandEvent &event);
  void view_togglediscovered(wxCommandEvent &event);
  void view_togglebookmarks(wxCommandEvent &event);
  void view_togglestatistics(wxCommandEvent &event);
  void view_togglefullscreen(wxCommandEvent &event);

  void bookmarks_add(wxCommandEvent &event); 
  void bookmarks_edit(wxCommandEvent &event); 
  void bookmarks_delete(wxCommandEvent &event);

  void help_about(wxCommandEvent &event);
  void help_contents(wxCommandEvent &event);

  // to be called from the App
  bool cmdline_connect(wxString& hostarg);

};



#endif
