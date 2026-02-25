// -*- C++ -*- 

#ifndef MYFRAMEMAIN_H
#define MYFRAMEMAIN_H


#include <vector>
#include <set>
#include <wx/process.h>
#include <wx/secretstore.h>
#include <wx/uri.h>
#include "FrameMain.h"
#include "MyFrameLog.h"
#include "wxServDisc.h"
#include "VNCConn.h"
#include "ViewerWindow.h"
#include "VNCSeamlessConnector.h"



// this hold all info regarding one connection
struct ConnBlob
{
  VNCConn* conn;
  ViewerWindow* viewerwindow;
  VNCSeamlessConnector* seamlessconnector;
  wxProcess* windowshare_proc;
  long windowshare_proc_pid; // this should be saved in the wxProcess, but isn't
};


class MyFrameMain: public FrameMain
{
  // main service scanner
  wxServDisc* servscan;

  // array of connections
  std::vector<ConnBlob> connections;
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
  int  show_seamless;
  bool show_1to1;

  // log window
  MyFrameLog* logwindow;

  void splitwinlayout();

  bool bookmarks_load_to_list();
  wxSecretString bookmarks_load_one(int index);
  void bookmarks_secrets_save(const wxString& bookmarkName,
                              const wxSecretValue& password,
                              const wxSecretValue& sshPassword,
                              const wxSecretValue& sshPrivKeyPassword);
  void bookmarks_secrets_load(const wxString& bookmarkName,
                              wxSecretValue& password,
                              wxSecretValue& sshPassword,
                              wxSecretValue& sshPrivKeyPassword);
  void bookmarks_secrets_delete(const wxString& bookmarkName);

  void ssh_known_hosts_save(const wxString& host, const wxString& port, const wxString& fingerprint);
  wxString ssh_known_hosts_load(const wxString& host, const wxString& port);
  wxString hex_to_base64(const wxString& hex);
  void x509_known_hosts_save(const wxString& host, const wxString& port, const wxString& fingerprint);
  wxString x509_known_hosts_load(const wxString& host, const wxString& port);
  wxString hex_to_colon_separated(const wxString& hex);

  // service can be user@host:port notation or a full vnc:// URI
  void conn_spawn(const wxString& service, int listenPort = -1);
  void conn_setup(VNCConn *conn);
  void conn_terminate(int which);

  // collab features
  wxString windowshare_cmd_template;
  void onWindowshareTerminate(wxProcessEvent& event);

  // helpers
  wxString getQueryValue(const wxURI& wxUri, const wxString& key);
  void parseHostString(const char *server, int defaultport, char **host, int *port);

  // private handlers
  void onMyFrameLogCloseNotify(wxCommandEvent& event);
  void onVNCConnListenNotify(wxCommandEvent& event);
  void onVNCConnInitNotify(wxCommandEvent& event);
  void onVNCConnGetPasswordNotify(wxCommandEvent& event);
  void onVNCConnGetCredentialsNotify(wxCommandEvent& event);
  void onVNCConnSshFingerprintMismatchNotify(wxCommandEvent &event);
  void onVNCConnX509FingerprintMismatchNotify(VNCConnX509FingerprintMismatchNotifyEvent &event);
  void onVNCConnUpdateNotify(VNCConnUpdateNotifyEvent& event);
  void onVNCConnUniMultiChangedNotify(wxCommandEvent& event);
  void onVNCConnReplayFinishedNotify(wxCommandEvent& event);
  void onVNCConnFBResizeNotify(wxCommandEvent& event);
  void onVNCConnDisconnectNotify(wxCommandEvent& event);
  void onVNCConnIncomingConnectionNotify(wxCommandEvent& event);
  void onVNCConnCuttextNotify(wxCommandEvent& event);
  void onVNCConnBellNotify(wxCommandEvent& event);
  void onSDNotify(wxCommandEvent& event);
  void onFullScreenChanged(wxFullScreenEvent &event);
  void onSysColourChanged(wxSysColourChangedEvent& event);
  void onSize(wxSizeEvent& event);

  bool saveStats(VNCConn* c, int conn_index, const wxArrayString& stats, wxString desc, bool autosave);

  // multi-sync input
  bool multi_sync_enabled;
  void updateMultiSyncTargets();


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
  void listbox_bookmarks_context(wxContextMenuEvent &event);

  void notebook_connections_pagechanged(wxNotebookEvent &event);

  void machine_connect(wxCommandEvent &event);
  void machine_listen(wxCommandEvent &event);
  void machine_disconnect(wxCommandEvent &event);
  void machine_preferences(wxCommandEvent &event);
  void machine_showlog(wxCommandEvent &event);
  void machine_screenshot(wxCommandEvent &event);
  void machine_grabkeyboard(wxCommandEvent &event);
  void machine_save_stats(wxCommandEvent &event); 
  void machine_input_record(wxCommandEvent &event);
  void machine_input_replay(wxCommandEvent &event);
  void machine_multisync(wxCommandEvent &event);
  void machine_exit(wxCommandEvent &event);

  void view_toggletoolbar(wxCommandEvent &event);
  void view_togglediscovered(wxCommandEvent &event);
  void view_togglebookmarks(wxCommandEvent &event);
  void view_togglestatistics(wxCommandEvent &event);
  void view_togglefullscreen(wxCommandEvent &event);
  void view_toggle1to1(wxCommandEvent &event);
  void view_seamless(wxCommandEvent &event);

  void bookmarks_add(wxCommandEvent &event); 
  void bookmarks_edit(wxCommandEvent &event); 
  void bookmarks_delete(wxCommandEvent &event);

  void windowshare_start(wxCommandEvent &event);
  void windowshare_stop(wxCommandEvent &event);

  void help_about(wxCommandEvent &event);
  void help_contents(wxCommandEvent &event);
  void help_issue_list(wxCommandEvent &event);

  // to be called from the App
  void cmdline_conn_spawn(const wxString& service, int listenPort);

};



#endif
