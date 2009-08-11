// -*- C++ -*- 

#ifndef MYFRAMEMAIN_H
#define MYFRAMEMAIN_H



#include "FrameMain.h"
#include "../wxServDisc/wxServDisc.h"
#include "../VNCConn.h"




class MyFrameMain: public FrameMain
{
  // main service scanner
  wxServDisc* servscan;

  // these are used by spawn_conn()
  wxString sc_hostname;
  wxString sc_addr;
  wxString sc_port;


  // gui layout stuff
  bool show_toolbar;
  bool show_discovered;
  bool show_bookmarks;
  bool show_stats;
  bool show_fullscreen;

  void splitwinlayout();

  bool spawn_conn();
  void terminate_conn();
 
  // private handlers
  void onVNCConnDisconnectNotify(wxCommandEvent& event);
  void onSDNotify(wxCommandEvent& event);
  
  static char* getpasswd(rfbClient* client);

  
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
  void machine_disconnect(wxCommandEvent &event);
  void machine_preferences(wxCommandEvent &event);
  void machine_exit(wxCommandEvent &event);

  void view_toggletoolbar(wxCommandEvent &event);
  void view_togglediscovered(wxCommandEvent &event);
  void view_togglebookmarks(wxCommandEvent &event);
  void view_togglestatistics(wxCommandEvent &event);
  void view_togglefullscreen(wxCommandEvent &event);

  void bookmarks(wxCommandEvent &event);

  void help_about(wxCommandEvent &event);
  void help_contents(wxCommandEvent &event);

};



#endif
