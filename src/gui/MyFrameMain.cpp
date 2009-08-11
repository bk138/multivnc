
#include "wx/aboutdlg.h"
#include "wx/socket.h"

#include "res/about.png.h"

#include "MyFrameMain.h"
#include "MyDialogSettings.h"
#include "../dfltcfg.h"
#include "../MultiVNCApp.h"




//FIXME
VNCConn *c;



// map recv of cusotm events to handler methods
BEGIN_EVENT_TABLE(MyFrameMain, FrameMain)
  EVT_COMMAND (wxID_ANY, wxServDiscNOTIFY, MyFrameMain::onSDNotify)
  EVT_COMMAND (wxID_ANY, VNCConnDisconnectNOTIFY, MyFrameMain::onVNCConnDisconnectNotify)
END_EVENT_TABLE()




using namespace std;
 









MyFrameMain::MyFrameMain(wxWindow* parent, int id, const wxString& title, 
			 const wxPoint& pos,
			 const wxSize& size,
			 long style):
  FrameMain(parent, id, title, pos, size, style)	
{
  // get default config object, created on demand if not exist
  wxConfigBase *pConfig = wxConfigBase::Get();
  pConfig->Read(K_SHOWTOOLBAR, &show_toolbar, V_SHOWTOOLBAR);
  pConfig->Read(K_SHOWDISCOVERED, &show_discovered, V_SHOWDISCOVERED);
  pConfig->Read(K_SHOWBOOKMARKS, &show_bookmarks, V_SHOWBOOKMARKS);
  pConfig->Read(K_SHOWSTATS, &show_stats, V_SHOWSTATS);

  show_fullscreen = false;
  splitwin_main->SetMinimumPaneSize(150);
  splitwin_left->SetMinimumPaneSize(75);
  splitwin_leftlower->SetMinimumPaneSize(75);



  /*
    setup menu items for a the frame
    unfortunately there seems to be a bug in wxMenu::FindItem(str): 
    it skips '&' characters,  but GTK uses '_' for accelerators and 
    these are not trimmed...
  */
  // "discconnect"
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(1)->Enable(false);

  if(show_toolbar)
     frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->FindItemByPosition(0)->Check();
  else
    {
      // disable the toolbar
      wxToolBar *tbar = GetToolBar();
      delete tbar;
      SetToolBar(NULL);
    }

  
  splitwinlayout();

  if(show_discovered)
    frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->FindItemByPosition(1)->Check();
  if(show_bookmarks)
    frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->FindItemByPosition(2)->Check();
  if(show_stats)
    frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->FindItemByPosition(3)->Check();


  // finally, our mdns service scanner
  servscan = new wxServDisc(this, wxT("_rfb._tcp.local."), QTYPE_PTR);
}




MyFrameMain::~MyFrameMain()
{
  terminate_conn();

}



/*
  private functions

*/



// handlers
void MyFrameMain::onVNCConnDisconnectNotify(wxCommandEvent& event)
{
  fprintf(stderr, "disconnect recvd!\n");

  wxLogStatus( _("Connection terminated."));
  wxLogMessage( _("Connection terminated."));
     
  // "end connection"
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(1)->Enable(false);
}



void MyFrameMain::onSDNotify(wxCommandEvent& event)
{
    if(event.GetEventObject() == servscan)
      {
	wxArrayString items; 
	
	// length of qeury plus leading dot
	size_t qlen =  servscan->getQuery().Len() + 1;
	
	vector<wxSDEntry> entries = servscan->getResults();
	vector<wxSDEntry>::const_iterator it; 
	for(it=entries.begin(); it != entries.end(); it++)
	  items.Add(it->name.Mid(0, it->name.Len() - qlen));
	
	list_box_services->Set(items, 0);
      }
}




char* MyFrameMain::getpasswd(rfbClient* client)
{
  wxString s = wxGetPasswordFromUser(_("Enter password:"),
				     _("Password required!"));
  return strdup(s.char_str());
}


// connection initiation and shutdown
bool MyFrameMain::spawn_conn()
{
  wxLogStatus(_("Connecting to ") + sc_addr + _T(":") + sc_port + _T("..."));
  wxBusyCursor busy;

  c = new VNCConn(this);
  if(!c->Init(sc_addr + _T(":") + sc_port, getpasswd))
    {
      wxLogStatus( _("Connection failed."));
      wxArrayString log = c->getLog();
      // show last 3 log strings
      for(int i = log.GetCount() - 3; i < log.GetCount(); ++i)
	wxLogMessage(log[i]);

      wxLogError(c->getErr());

      return false;
    }
 
  wxLogStatus(_("Connected to ") + sc_addr + _T(":") + sc_port);


  // "end connection"
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(1)->Enable(true);
  
  return true;
}


void MyFrameMain::terminate_conn()
{
  if(c)
    c->Shutdown();
  delete c;
  c = 0;
	  
  // "end connection"
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(1)->Enable(false);

  wxLogStatus( _("Connection terminated."));
}






void MyFrameMain::splitwinlayout()
{
  // setup layout in respect to every possible combination
  
  if(!show_discovered && !show_bookmarks && !show_stats) // 000
    {
      splitwin_main->Unsplit(splitwin_main_pane_1);
    }
 
  if(!show_discovered && !show_bookmarks && show_stats) // 001
    {
      splitwin_main->SplitVertically(splitwin_main_pane_1, splitwin_main_pane_2);

      splitwin_left->SplitHorizontally(splitwin_left_pane_1, splitwin_left_pane_2);
      splitwin_left->Unsplit(splitwin_left_pane_1);

      splitwin_leftlower->SplitHorizontally(splitwin_leftlower_pane_1, splitwin_leftlower_pane_2);
      splitwin_leftlower->Unsplit(splitwin_leftlower_pane_1);
    }
 
  if(!show_discovered && show_bookmarks && !show_stats)  // 010
    {
      splitwin_main->SplitVertically(splitwin_main_pane_1, splitwin_main_pane_2);

      splitwin_left->SplitHorizontally(splitwin_left_pane_1, splitwin_left_pane_2);
      splitwin_left->Unsplit(splitwin_left_pane_1);

      splitwin_leftlower->SplitHorizontally(splitwin_leftlower_pane_1, splitwin_leftlower_pane_2);
      splitwin_leftlower->Unsplit(splitwin_leftlower_pane_2);
    }
 
  if(!show_discovered && show_bookmarks && show_stats)  // 011
    {
      splitwin_main->SplitVertically(splitwin_main_pane_1, splitwin_main_pane_2);

      splitwin_left->SplitHorizontally(splitwin_left_pane_1, splitwin_left_pane_2);
      splitwin_left->Unsplit(splitwin_left_pane_1);

      splitwin_leftlower->SplitHorizontally(splitwin_leftlower_pane_1, splitwin_leftlower_pane_2);
    }
 
  if(show_discovered && !show_bookmarks && !show_stats) // 100
    {
      splitwin_main->SplitVertically(splitwin_main_pane_1, splitwin_main_pane_2);

      splitwin_left->SplitHorizontally(splitwin_left_pane_1, splitwin_left_pane_2);
      splitwin_left->Unsplit(splitwin_left_pane_2);
    }
 
  if(show_discovered && !show_bookmarks && show_stats) // 101
    {
      splitwin_main->SplitVertically(splitwin_main_pane_1, splitwin_main_pane_2);

      splitwin_left->SplitHorizontally(splitwin_left_pane_1, splitwin_left_pane_2);

      splitwin_leftlower->SplitHorizontally(splitwin_leftlower_pane_1, splitwin_leftlower_pane_2);
      splitwin_leftlower->Unsplit(splitwin_leftlower_pane_1);
    }
 
  if(show_discovered && show_bookmarks && !show_stats) // 110
    {
      splitwin_main->SplitVertically(splitwin_main_pane_1, splitwin_main_pane_2);

      splitwin_left->SplitHorizontally(splitwin_left_pane_1, splitwin_left_pane_2);
      
      splitwin_leftlower->SplitHorizontally(splitwin_leftlower_pane_1, splitwin_leftlower_pane_2);
      splitwin_leftlower->Unsplit(splitwin_leftlower_pane_2);
    }
 
  if(show_discovered && show_bookmarks && show_stats) // 111
    {
      splitwin_main->SplitVertically(splitwin_main_pane_1, splitwin_main_pane_2);
      splitwin_left->SplitHorizontally(splitwin_left_pane_1, splitwin_left_pane_2);
      splitwin_leftlower->SplitHorizontally(splitwin_leftlower_pane_1, splitwin_leftlower_pane_2);
    }

  // and set proportions
  int w,h;
  GetSize(&w, &h);

  splitwin_main->SetSashPosition(w * 0.1);

  if(show_bookmarks)
    splitwin_left->SetSashPosition(h * 0.4);
  else
    splitwin_left->SetSashPosition(h * 0.9);

  splitwin_leftlower->SetSashPosition(h * 0.75);
}




/*
  handler functions
*/





void MyFrameMain::machine_connect(wxCommandEvent &event)
{
  wxString s = wxGetTextFromUser(_("Enter host to connect to:"),
				 _("Connect to specific host"));
				
  if(s != wxEmptyString)
    {
      wxIPV4address host_addr;
      
      // get host part and port part
      wxString host_name, host_port;
      host_name = s.BeforeFirst(_T(':'));
      host_port = s.AfterFirst(_T(':'));
      
      // look up name
      if(! host_addr.Hostname(host_name))
	{
	  wxLogError(_("Invalid hostname or IP address."));
	  sc_addr = sc_hostname = sc_port = wxEmptyString; 
	  return;
	}
      else
#ifdef __WIN32__
	sc_addr = host_addr.Hostname(); // wxwidgets bug, ah well ...
#else
        sc_addr = host_addr.IPAddress();
#endif
  
      
      // and handle port
      if(host_addr.Service(host_port))
	sc_port = wxString() << host_addr.Service();
      else
	sc_port = wxEmptyString;
      
      spawn_conn();
    }
}




void MyFrameMain::machine_disconnect(wxCommandEvent &event)
{
  terminate_conn();
}






void MyFrameMain::machine_preferences(wxCommandEvent &event)
{
  MyDialogSettings dialog_settings(this, wxID_ANY, _("Preferences"));
 
  if(dialog_settings.ShowModal() == wxID_OK)
    {
      wxConfigBase *pConfig = wxConfigBase::Get();      

      
      //pConfig->Write(K_CUSTOMVIEWER, dialog_settings.getViewer());

      //read_config();
    }
}



void MyFrameMain::machine_exit(wxCommandEvent &event)
{
  terminate_conn();
  Close(true);
}




void MyFrameMain::view_toggletoolbar(wxCommandEvent &event)
{
  show_toolbar = !show_toolbar;

  wxToolBar *tbar = GetToolBar();

  if (!tbar)
    {
      // this is copied over from the FrameMain constructor
      // not nice, but wtf...
      frame_main_statusbar = CreateStatusBar(2, 0);
      frame_main_toolbar = new wxToolBar(this, -1, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL|wxTB_DOCKABLE|wxTB_3DBUTTONS|wxTB_TEXT);
      SetToolBar(frame_main_toolbar);
      frame_main_toolbar->SetToolBitmapSize(wxSize(24, 24));
      frame_main_toolbar->AddTool(wxID_YES, _("Connect"), (bitmapFromMem(connect_png)), (bitmapFromMem(connect_png)), wxITEM_NORMAL, wxEmptyString, wxEmptyString);
      frame_main_toolbar->AddTool(wxID_CANCEL, _("Disconnect"), (bitmapFromMem(disconnect_png)), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
      frame_main_toolbar->AddSeparator();
      frame_main_toolbar->AddTool(wxID_ZOOM_FIT, _("Fullscreen"), (bitmapFromMem(fullscreen_png)), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
      frame_main_toolbar->AddTool(wxID_SAVE, _("Take Screenshot"), (bitmapFromMem(screenshot_png)), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
      frame_main_toolbar->Realize();
    }
  else
    {
      delete tbar;
      SetToolBar(NULL);
    }


  wxConfigBase *pConfig = wxConfigBase::Get();
  pConfig->Write(K_SHOWTOOLBAR, show_toolbar);
}



void MyFrameMain::view_togglediscovered(wxCommandEvent &event)
{
  show_discovered = !show_discovered;

  splitwinlayout();
 
  wxConfigBase *pConfig = wxConfigBase::Get();
  pConfig->Write(K_SHOWDISCOVERED, show_discovered);
}


void MyFrameMain::view_togglebookmarks(wxCommandEvent &event)
{
  show_bookmarks = !show_bookmarks;

  splitwinlayout();
 
  wxConfigBase *pConfig = wxConfigBase::Get();
  pConfig->Write(K_SHOWBOOKMARKS, show_bookmarks);
}



void MyFrameMain::view_togglestatistics(wxCommandEvent &event)
{
  show_stats = !show_stats;

  splitwinlayout();

  wxConfigBase *pConfig = wxConfigBase::Get();
  pConfig->Write(K_SHOWSTATS, show_stats);
}









void MyFrameMain::view_togglefullscreen(wxCommandEvent &event)
{
  show_fullscreen = ! show_fullscreen;

  ShowFullScreen(show_fullscreen, 
		 wxFULLSCREEN_NOTOOLBAR | wxFULLSCREEN_NOSTATUSBAR | wxFULLSCREEN_NOBORDER | wxFULLSCREEN_NOCAPTION);

  if(show_fullscreen)
    frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->FindItemByPosition(5)->Check();
}



void MyFrameMain::help_about(wxCommandEvent &event)
{	
  wxAboutDialogInfo info;
  wxIcon icon;
  icon.CopyFromBitmap(bitmapFromMem(about_png));

  info.SetIcon(icon);
  info.SetName(wxT("MultiVNC"));
  info.SetVersion(wxT(VERSION));
  info.SetDescription(_("MultiVNC is a cross-platform Multicast-enabled VNC client."));
  info.SetCopyright(wxT(COPYRIGHT));
  
  wxAboutBox(info); 
}




void MyFrameMain::help_contents(wxCommandEvent &e)
{
  wxLogMessage(_("Select a host in the list to get more information, double click to connect.\n\nWhen in Remote Control Only Mode, move the pointer over the right screen edge to control the remote desktop."));
}











void MyFrameMain::listbox_services_select(wxCommandEvent &event)
{
  int timeout;
  wxBusyCursor busy;
  int sel = event.GetInt();
 
  if(sel < 0) // seems this happens when we update the list
    return;

  wxLogStatus(_("Looking up host address..."));


  // lookup hostname and port
  {
    wxServDisc namescan(0, servscan->getResults().at(sel).name, QTYPE_SRV);
  
    timeout = 3000;
    while(!namescan.getResultCount() && timeout > 0)
      {
	wxMilliSleep(25);
	timeout-=25;
      }
    if(timeout <= 0)
      {
	wxLogError(_("Timeout looking up hostname."));
	wxLogStatus(_("Timeout looking up hostname."));
	sc_hostname = sc_addr = sc_port = wxEmptyString;
	return;
      }
    sc_hostname = namescan.getResults().at(0).name;
    sc_port = wxString() << namescan.getResults().at(0).port;
  }

  
  // lookup ip address
  {
    wxServDisc addrscan(0, sc_hostname, QTYPE_A);
  
    timeout = 3000;
    while(!addrscan.getResultCount() && timeout > 0)
      {
	wxMilliSleep(25);
	timeout-=25;
      }
    if(timeout <= 0)
      {
	wxLogError(_("Timeout looking up IP address."));
	wxLogStatus(_("Timeout looking up IP address."));
	sc_hostname = sc_addr = sc_port = wxEmptyString;
	return;
      }
    sc_addr = addrscan.getResults().at(0).ip;
  }

  wxLogStatus(sc_hostname + wxT(" (") + sc_addr + wxT(":") + sc_port + wxT(")"));
}


void MyFrameMain::listbox_services_dclick(wxCommandEvent &event)
{
  listbox_services_select(event); // get the actual values
  spawn_conn();
} 
 

void MyFrameMain::listbox_bookmarks_select(wxCommandEvent &event)
{
}


void MyFrameMain::listbox_bookmarks_dclick(wxCommandEvent &event)
{
}
