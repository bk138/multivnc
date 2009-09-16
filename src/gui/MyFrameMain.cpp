
#include "wx/aboutdlg.h"
#include "wx/socket.h"

#include "res/about.png.h"

#include "MyFrameMain.h"
#include "MyDialogSettings.h"
#include "../dfltcfg.h"
#include "../MultiVNCApp.h"


using namespace std;



/********************************************

  VNCCanvas class

********************************************/
	
// define a scrollable canvas for drawing onto
class VNCCanvas: public wxScrolledWindow
{
  VNCConn* conn;
  wxRect framebuffer_rect;


  void onPaint(wxPaintEvent &event);
  void onMouseMove(wxMouseEvent &event);

protected:
  DECLARE_EVENT_TABLE()

public:
  VNCCanvas(wxWindow* parent, VNCConn* c);

  void drawRegion(wxRect& rect);

};
	


BEGIN_EVENT_TABLE(VNCCanvas, wxScrolledWindow)
    EVT_PAINT  (VNCCanvas::onPaint)
    EVT_MOTION (VNCCanvas::onMouseMove)
END_EVENT_TABLE();

#define VNCCANVAS_SCROLL_RATE 10

/*
  constructor/destructor
*/

VNCCanvas::VNCCanvas(wxWindow* parent, VNCConn* c):
  wxScrolledWindow(parent)
{
  conn = c;
  framebuffer_rect.x = 0;
  framebuffer_rect.y = 0;
  framebuffer_rect.width = c->getFrameBufferWidth();
  framebuffer_rect.height = c->getFrameBufferHeight();

  SetVirtualSize(framebuffer_rect.width, framebuffer_rect.height);
  SetScrollRate(VNCCANVAS_SCROLL_RATE, VNCCANVAS_SCROLL_RATE);
}



/*
  private members
*/

void VNCCanvas::onPaint(wxPaintEvent &WXUNUSED(event))
{
  wxPaintDC dc(this);

  // find out where the window is scrolled to
  static int rx, ry;                    
  GetViewStart(&rx, &ry);
  rx *= VNCCANVAS_SCROLL_RATE;
  ry *= VNCCANVAS_SCROLL_RATE;

  
  // get the update rect list
  wxRegionIterator upd(GetUpdateRegion()); 
  while(upd)
    {
      wxRect viewport_rect(upd.GetRect());
     
      wxLogDebug(wxT("VNCCanvas %p: got repaint event: on screen (%i,%i,%i,%i)"),
		 this,
		 viewport_rect.x,
		 viewport_rect.y,
		 viewport_rect.width,
		 viewport_rect.height);

      wxRect fromframebuffer_rect = viewport_rect;
      fromframebuffer_rect.x += rx;
      fromframebuffer_rect.y += ry;
      fromframebuffer_rect.Intersect(framebuffer_rect);
      
      wxLogDebug(wxT("VNCCanvas %p: got repaint event: in framebuffer (%i,%i,%i,%i)"),
		 this,
		 fromframebuffer_rect.x,
		 fromframebuffer_rect.y,
		 fromframebuffer_rect.width,
		 fromframebuffer_rect.height);
      
      if(! fromframebuffer_rect.IsEmpty())
	{
      	  wxBitmap region = conn->getFrameBufferRegion(fromframebuffer_rect);
	  dc.DrawBitmap(region, viewport_rect.x, viewport_rect.y);
	}

      ++upd;
    }
}



void VNCCanvas::onMouseMove(wxMouseEvent &event)
{
  wxClientDC dc(this);
  PrepareDC(dc);

  wxPoint pos = event.GetPosition();
  long x = dc.DeviceToLogicalX( pos.x );
  long y = dc.DeviceToLogicalY( pos.y );
  
  // dc.DrawBitmap(cursorbmp, pos.x, pos.y);

  wxLogDebug(wxT("VNCCanvas %p: mouse position: %d,%d"), this, (int)x, (int)y);
}




/*
  public members
*/
void VNCCanvas::drawRegion(wxRect& rect)
{
  wxClientDC dc(this);
  DoPrepareDC(dc); // this adjusts coordinates when window is scrolled
  wxBitmap region = conn->getFrameBufferRegion(rect);
  dc.DrawBitmap(region, rect.x, rect.y);
}







/********************************************

  MyFrameMain class

********************************************/



// map recv of custom events to handler methods
BEGIN_EVENT_TABLE(MyFrameMain, FrameMain)
  EVT_COMMAND (wxID_ANY, MyFrameLogCloseNOTIFY, MyFrameMain::onMyFrameLogCloseNotify)
  EVT_COMMAND (wxID_ANY, wxServDiscNOTIFY, MyFrameMain::onSDNotify)
  EVT_COMMAND (wxID_ANY, VNCConnUpdateNOTIFY, MyFrameMain::onVNCConnUpdateNotify)
  EVT_COMMAND (wxID_ANY, VNCConnDisconnectNOTIFY, MyFrameMain::onVNCConnDisconnectNotify)
END_EVENT_TABLE()


/*
  constructor/destructor
*/

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
  SetMinSize(wxSize(512, 384));
  splitwin_main->SetMinimumPaneSize(150);
  splitwin_left->SetMinimumPaneSize(140);
  splitwin_leftlower->SetMinimumPaneSize(70);


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


  // theres no log window at startup
  logwindow = 0;

  // finally, our mdns service scanner
  servscan = new wxServDisc(this, wxT("_rfb._tcp.local."), QTYPE_PTR);
}




MyFrameMain::~MyFrameMain()
{
  for(int i=0; i < connections.size(); ++i)
    terminate_conn(i);

  delete servscan;
}





/*
  private members
*/


// handlers

void MyFrameMain::onMyFrameLogCloseNotify(wxCommandEvent& event)
{
  logwindow = 0;
}




void MyFrameMain::onVNCConnUpdateNotify(wxCommandEvent& event)
{
  // only process currently selected connection
  VNCConn* c = connections.at(notebook_connections->GetSelection());
  if(c == event.GetEventObject())
    {
      wxRect* rect = static_cast<wxRect*>(event.GetClientData());

      wxLogDebug(wxT("active page got upd (%i,%i,%i,%i)"),
		 rect->x,
		 rect->y,
		 rect->width,
		 rect->height);

      VNCCanvas* canvas = static_cast<VNCCanvas*>(notebook_connections->GetCurrentPage());
      canvas->drawRegion(*rect);
      
      // avoid memleaks!
      delete rect;
    }
}



void MyFrameMain::onVNCConnDisconnectNotify(wxCommandEvent& event)
{
  // get sender
  VNCConn* c = static_cast<VNCConn*>(event.GetEventObject());

  wxLogStatus( _("Connection to %s:%s terminated."), c->getServerName().c_str(), c->getServerPort().c_str());
 
  wxArrayString log = VNCConn::getLog();
  // show last 3 log strings
  for(int i = log.GetCount() >= 3 ? log.GetCount()-3 : 0; i < log.GetCount(); ++i)
    wxLogMessage(log[i]);
  wxLogMessage( _("Connection to %s:%s terminated."), c->getServerName().c_str(), c->getServerPort().c_str() );
    
  
  // find index of this connection
  vector<VNCConn*>::iterator it = connections.begin();
  int index = 0;
  while(it != connections.end() && *it != c)
    {
      ++it;
      ++index;
    }

  if(index < connections.size())
    {
      delete c;
      connections.erase(connections.begin() + index);

      notebook_connections->DeletePage(index);
    }

  // "end connection"
  if(connections.size() == 0) // nothing to end
    frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(1)->Enable(false);
}



void MyFrameMain::onSDNotify(wxCommandEvent& event)
{
  if(event.GetEventObject() == servscan)
    {
      wxArrayString items; 
      
      // length of query plus leading dot
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
bool MyFrameMain::spawn_conn(wxString& hostname, wxString& addr, wxString& port)
{
  wxLogStatus(_("Connecting to ") + hostname + _T(":") + port + _T("..."));
  wxBusyCursor busy;
  

  VNCConn* c = new VNCConn(this);
  if(!c->Init(addr + wxT(":") + port, getpasswd))
    {
      wxLogStatus( _("Connection failed."));
      wxArrayString log = VNCConn::getLog();
      // show last 3 log strings
      for(int i = log.GetCount() >= 3 ? log.GetCount() - 3 : 0; i < log.GetCount(); ++i)
	wxLogMessage(log[i]);

      wxLogError(c->getErr());

      delete c;

      return false;
    }
  
  connections.push_back(c);

  VNCCanvas* canvas = new VNCCanvas(notebook_connections, c);
  notebook_connections->AddPage(canvas, hostname, true);

  wxLogStatus(_("Connected to ") + hostname + _T(":") + port);


  // "end connection"
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(1)->Enable(true);
  
  return true;
}


void MyFrameMain::terminate_conn(size_t which)
{
  if(which == wxNOT_FOUND)
    return;

  VNCConn* c = connections.at(which);
  if(c != 0)
    {
      delete c;
      connections.erase(connections.begin() + which);

      notebook_connections->DeletePage(which);
    }

	  
  // "end connection"
  if(connections.size() == 0) // nothing to end
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
  public members
*/


void MyFrameMain::machine_connect(wxCommandEvent &event)
{
  wxString s = wxGetTextFromUser(_("Enter host to connect to:"),
				 _("Connect to specific host"));
				
  if(s != wxEmptyString)
    {
      wxIPV4address host_addr;

      wxString sc_hostname, sc_port, sc_addr;      

      // get host part 
      sc_hostname = s.BeforeFirst(wxT(':'));

      // look up name
      if(! host_addr.Hostname(sc_hostname))
	{
	  wxLogError(_("Invalid hostname or IP address."));
	  return;
	}
      else
#ifdef __WIN32__
	sc_addr = host_addr.Hostname(); // wxwidgets bug, ah well ...
#else
        sc_addr = host_addr.IPAddress();
#endif
      
      // and lookup port description part (sth. like 'vnc' can also be given)
      if(host_addr.Service( s.AfterFirst(wxT(':')) ))
	sc_port = wxString() << host_addr.Service();
      else
	sc_port = wxEmptyString;

      spawn_conn(sc_hostname, sc_addr, sc_port);
    }
}




void MyFrameMain::machine_disconnect(wxCommandEvent &event)
{
  // terminate connection thats currently selected
  terminate_conn(notebook_connections->GetSelection());
}




void MyFrameMain::machine_showlog(wxCommandEvent &event)
{
  if(!logwindow)
    {
      logwindow = new MyFrameLog(this, wxID_ANY, _("Detailed VNC Log"));
      logwindow->Show();
    }
  else
    logwindow->Raise();
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


void MyFrameMain::machine_screenshot(wxCommandEvent &event)
{
  /* if(c)
    {
      wxRect rect(0, 0, c->getFrameBufferWidth(), c->getFrameBufferHeight());
      wxBitmap shot = c->getFrameBufferRegion(rect);
      wxString filename = wxFileSelector(_("Save screenshot..."), wxEmptyString, wxT("MultiVNC-Screenshot.png"), 
					 wxT(".png"), wxT("*.png"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

      if(!filename.empty())
	{
	  wxBusyCursor busy;
	  shot.SaveFile(filename, wxBITMAP_TYPE_PNG);
	}
	}
  */
}




void MyFrameMain::machine_exit(wxCommandEvent &event)
{
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
	services_hostname = services_addr = services_port = wxEmptyString;
	return;
      }
    services_hostname = namescan.getResults().at(0).name;
    services_port = wxString() << namescan.getResults().at(0).port;
  }

  
  // lookup ip address
  {
    wxServDisc addrscan(0, services_hostname, QTYPE_A);
  
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
	services_hostname = services_addr = services_port = wxEmptyString;
	return;
      }
    services_addr = addrscan.getResults().at(0).ip;
  }

  wxLogStatus(services_hostname + wxT(" (") + services_addr + wxT(":") + services_port + wxT(")"));
}


void MyFrameMain::listbox_services_dclick(wxCommandEvent &event)
{
  listbox_services_select(event); // get the actual values
  spawn_conn(services_hostname, services_addr, services_port);
} 
 

void MyFrameMain::listbox_bookmarks_select(wxCommandEvent &event)
{
}


void MyFrameMain::listbox_bookmarks_dclick(wxCommandEvent &event)
{
}
