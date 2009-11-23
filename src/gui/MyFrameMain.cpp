
#include <fstream>
#include <wx/aboutdlg.h>
#include <wx/socket.h>
#include <wx/clipbrd.h>

#include "res/about.png.h"

#include "MyFrameMain.h"
#include "MyDialogSettings.h"
#include "VNCCanvas.h"
#include "../dfltcfg.h"
#include "../MultiVNCApp.h"

#define STATS_TIMER_INTERVAL 50


using namespace std;


// map recv of custom events to handler methods
BEGIN_EVENT_TABLE(MyFrameMain, FrameMain)
  EVT_COMMAND (wxID_ANY, MyFrameLogCloseNOTIFY, MyFrameMain::onMyFrameLogCloseNotify)
  EVT_COMMAND (wxID_ANY, wxServDiscNOTIFY, MyFrameMain::onSDNotify)
  EVT_COMMAND (wxID_ANY, VNCConnUpdateNOTIFY, MyFrameMain::onVNCConnUpdateNotify)
  EVT_COMMAND (wxID_ANY, VNCConnFBResizeNOTIFY, MyFrameMain::onVNCConnFBResizeNotify)
  EVT_COMMAND (wxID_ANY, VNCConnCuttextNOTIFY, MyFrameMain::onVNCConnCuttextNotify)
  EVT_COMMAND (wxID_ANY, VNCConnDisconnectNOTIFY, MyFrameMain::onVNCConnDisconnectNotify)
  EVT_COMMAND (wxID_ANY, VNCConnIncomingConnectionNOTIFY, MyFrameMain::onVNCConnIncomingConnectionNotify)
  EVT_TIMER   (wxID_ANY, MyFrameMain::onStatsTimer)
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
  int x,y;
  // get default config object, created on demand if not exist
  wxConfigBase *pConfig = wxConfigBase::Get();
  pConfig->Read(K_SHOWTOOLBAR, &show_toolbar, V_SHOWTOOLBAR);
  pConfig->Read(K_SHOWDISCOVERED, &show_discovered, V_SHOWDISCOVERED);
  pConfig->Read(K_SHOWBOOKMARKS, &show_bookmarks, V_SHOWBOOKMARKS);
  pConfig->Read(K_SHOWSTATS, &show_stats, V_SHOWSTATS);
  pConfig->Read(K_SIZE_X, &x, V_SIZE_X);
  pConfig->Read(K_SIZE_Y, &y, V_SIZE_Y);


  show_fullscreen = false;
  SetMinSize(wxSize(640, 480));
  splitwin_main->SetMinimumPaneSize(160);
  splitwin_left->SetMinimumPaneSize(250);
  splitwin_leftlower->SetMinimumPaneSize(120);

  SetSize(x, y);

  /*
    setup menu items for a the frame
    unfortunately there seems to be a bug in wxMenu::FindItem(str): 
    it skips '&' characters,  but GTK uses '_' for accelerators and 
    these are not trimmed...
  */
  // "disconnect"
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(2)->Enable(false);
  // "screenshot"
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(5)->Enable(false);
  // stats
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(6)->GetSubMenu()->FindItemByPosition(0)->Enable(false);
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(6)->GetSubMenu()->FindItemByPosition(1)->Enable(false);
  // bookmarks
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Bookmarks")))->FindItemByPosition(0)->Enable(false);
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Bookmarks")))->FindItemByPosition(2)->Enable(false);
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Bookmarks")))->FindItemByPosition(3)->Enable(false);
  

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

  loadbookmarks();

  if(show_discovered)
    frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->FindItemByPosition(1)->Check();
  if(show_bookmarks)
    frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->FindItemByPosition(2)->Check();
  if(show_stats)
    {
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->FindItemByPosition(3)->Check();
      stats_timer.Start(STATS_TIMER_INTERVAL);
    }

  // setup clipboard
#ifdef __WXGTK__
  // always use middle mouse button paste
  wxCriticalSectionLocker lock(wxGetApp().mutex_theclipboard); 
  if(wxTheClipboard->Open())
    {
      wxTheClipboard->UsePrimarySelection(true);
      wxTheClipboard->Close();
    }

#endif  
  

  // theres no log window at startup
  logwindow = 0;

  stats_timer.SetOwner(this);


  // finally, our mdns service scanner
  servscan = new wxServDisc(this, wxT("_rfb._tcp.local."), QTYPE_PTR);
}




MyFrameMain::~MyFrameMain()
{
  wxConfigBase *pConfig = wxConfigBase::Get();
  int x,y;
  GetSize(&x, &y);
  pConfig->Write(K_SIZE_X, x);
  pConfig->Write(K_SIZE_Y, y);

  bool save_stats;
  pConfig->Read(K_STATSAUTOSAVE, &save_stats, V_STATSAUTOSAVE);  
  if(save_stats)
    {
      for(int i = connections.size()-1; i >= 0; --i)
	{
	  VNCConn* c = connections.at(i);
	  wxString desktopname =  c->getDesktopName();
#ifdef __WIN32__
	  // windows doesn't like ':'s
	  desktopname.Replace(wxString(wxT(":")), wxString(wxT("-")));
#endif

	  if(! c->getUpdateStats().IsEmpty())
	    {
	      wxString filename = desktopname + wxT("-FramebufferUpdate-Stats-") + wxNow() + wxT(".txt");
	      if(! saveArrayString(const_cast<wxArrayString&>(c->getUpdateStats()), filename))
		wxLogError(_("Could not autosave framebuffer update statistics!"));
	    }
	  
	  if(! c->getLatencyStats().IsEmpty())
	    {
	      wxString filename = desktopname + wxT("-PointerLatency-Stats-") + wxNow() + wxT(".txt");
	      if(! saveArrayString(const_cast<wxArrayString&>(c->getLatencyStats()), filename))
		wxLogError(_("Could not autosave pointer latency statistics!"));
	    }

	  
	}
    }
 

  for(int i = connections.size()-1; i >= 0; --i)
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
      VNCCanvas* canvas = static_cast<VNCCanvasContainer*>(notebook_connections->GetCurrentPage())->getCanvas();
      canvas->drawRegion(*rect);
      delete rect; // avoid memleaks!
    }
}



void MyFrameMain::onVNCConnFBResizeNotify(wxCommandEvent& event)
{
  // get sender
  VNCConn* c = static_cast<VNCConn*>(event.GetEventObject());

  // find index of this connection
  vector<VNCConn*>::iterator it = connections.begin();
  size_t index = 0;
  while(it != connections.end() && *it != c)
    {
      ++it;
      ++index;
    }

  if(index < connections.size()) // found
    {
      VNCCanvas* canvas = static_cast<VNCCanvasContainer*>(notebook_connections->GetPage(index))->getCanvas();
      canvas->adjustSize();
    }
}



void MyFrameMain::onVNCConnCuttextNotify(wxCommandEvent& event)
{ 
  wxCriticalSectionLocker lock(wxGetApp().mutex_theclipboard); 
  if(wxTheClipboard->Open()) 
    {
      // get sender
      VNCConn* c = (VNCConn*)event.GetEventObject();
      // these data objects are held by the clipboard, so do not delete them in the app.
      wxTheClipboard->SetData(new wxTextDataObject(c->getCuttext()));
      wxTheClipboard->Close();
    }
}





void MyFrameMain::onVNCConnDisconnectNotify(wxCommandEvent& event)
{
  // get sender
  VNCConn* c = static_cast<VNCConn*>(event.GetEventObject());

  wxLogStatus( _("Connection to %s:%s terminated."), c->getServerName().c_str(), c->getServerPort().c_str());
 
  wxArrayString log = VNCConn::getLog();
  // show last 3 log strings
  for(size_t i = log.GetCount() >= 3 ? log.GetCount()-3 : 0; i < log.GetCount(); ++i)
    wxLogMessage(log[i]);
  wxLogMessage( _("Connection to %s:%s terminated."), c->getServerName().c_str(), c->getServerPort().c_str() );
    
  
  // find index of this connection
  vector<VNCConn*>::iterator it = connections.begin();
  size_t index = 0;
  while(it != connections.end() && *it != c)
    {
      ++it;
      ++index;
    }

  if(index < connections.size())
    terminate_conn(index);
}




void MyFrameMain::onVNCConnIncomingConnectionNotify(wxCommandEvent& event)
{ 
  wxLogStatus(_("Incoming Connection."));

  // get sender
  VNCConn* c = static_cast<VNCConn*>(event.GetEventObject());

  // get connection settings
  int compresslevel, quality;
  wxConfigBase *pConfig = wxConfigBase::Get();
  pConfig->Read(K_COMPRESSLEVEL, &compresslevel, V_COMPRESSLEVEL);
  pConfig->Read(K_QUALITY, &quality, V_QUALITY);

  if(!c->Init(wxEmptyString, compresslevel, quality))
    {
      wxLogStatus( _("Connection failed."));
      wxArrayString log = VNCConn::getLog();
      // show last 3 log strings
      for(size_t i = log.GetCount() >= 3 ? log.GetCount() - 3 : 0; i < log.GetCount(); ++i)
	wxLogMessage(log[i]);
      
      wxLogError(c->getErr());
    }
  else
    {
      // find index of this connection
      vector<VNCConn*>::iterator it = connections.begin();
      size_t index = 0;
      while(it != connections.end() && *it != c)
	{
	  ++it;
	  ++index;
	}

      if(index < connections.size())
	notebook_connections->SetPageText(index, c->getDesktopName() + _(" (Reverse Connection)"));
    }
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



void MyFrameMain::onStatsTimer(wxTimerEvent& event)
{
  if(connections.size())
    {
      VNCConn* c = connections.at(notebook_connections->GetSelection());

      text_ctrl_fps->Clear();
      text_ctrl_latency->Clear();
      
      if( ! c->getUpdateStats().IsEmpty() )
	*text_ctrl_fps << c->getUpdateStats().Last().AfterFirst(wxT(','));
      if( ! c->getLatencyStats().IsEmpty() )
	*text_ctrl_latency << c->getLatencyStats().Last().AfterFirst(wxT(','));
    }
}



char* MyFrameMain::getpasswd(rfbClient* client)
{
  wxString s = wxGetPasswordFromUser(_("Enter password:"),
				     _("Password required!"));
  return strdup(s.char_str());
}



bool MyFrameMain::saveArrayString(wxArrayString& arrstr, wxString& path)
{
  ofstream ostream(path.char_str());
  if(! ostream)
    return false;

  for(size_t i=0; i < arrstr.GetCount(); ++i)
    ostream << arrstr[i].char_str() << endl;

  return true;
}



// connection initiation and shutdown
bool MyFrameMain::spawn_conn(bool listen, wxString hostname, wxString addr, wxString port)
{
  wxBusyCursor busy;
  wxIPV4address host_addr;

  // port can also be something like 'vnc', so look it up...
  if(host_addr.Service(port))
    port = wxString() << host_addr.Service();
  else
    port = wxT("5900");
  

  // get connection settings
  int compresslevel, quality;
  wxConfigBase *pConfig = wxConfigBase::Get();
  pConfig->Read(K_COMPRESSLEVEL, &compresslevel, V_COMPRESSLEVEL);
  pConfig->Read(K_QUALITY, &quality, V_QUALITY);

  VNCConn* c = new VNCConn(this);
  c->Setup(getpasswd);

  if(listen)
    {
      wxLogStatus(_("Listening on port ") + port + wxT(" ..."));
      if(!c->Listen(wxAtoi(port)))
	{
	  wxLogError(c->getErr());
	  delete c;
	  return false;
	}
    }
  else // normal init without previous listen
    {
      // look up addr if it's not given
      if(addr.IsEmpty())
	{
	  if(! host_addr.Hostname(hostname))
	    {
	      wxLogError(_("Invalid hostname or IP address."));
	      delete c;
	      return false;
	    }
	  else
#ifdef __WIN32__
	  addr = host_addr.Hostname(); // wxwidgets bug, ah well ...
#else
	  addr = host_addr.IPAddress();
#endif
	}


      wxLogStatus(_("Connecting to ") + hostname + _T(":") + port + wxT(" ..."));
      if(!c->Init(addr + wxT(":") + port, compresslevel, quality))
	{
	  wxLogStatus( _("Connection failed."));
	  wxArrayString log = VNCConn::getLog();
	  // show last 3 log strings
	  for(size_t i = log.GetCount() >= 3 ? log.GetCount() - 3 : 0; i < log.GetCount(); ++i)
	    wxLogMessage(log[i]);
	  
	  wxLogError(c->getErr());
	  delete c;
	  return false;
	}
    }
  
  connections.push_back(c);

  if(show_stats)
    c->doStats(true);

  VNCCanvasContainer* container = new VNCCanvasContainer(notebook_connections);
  VNCCanvas* canvas = new VNCCanvas(container, c);
  container->setCanvas(canvas);
  if(listen)
    notebook_connections->AddPage(container, _("Listening on port ") + port, true);    
  else
    notebook_connections->AddPage(container, c->getDesktopName(), true);


  // "end connection"
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(2)->Enable(true);
  // "screenshot"
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(5)->Enable(true);
  // stats
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(6)->GetSubMenu()->FindItemByPosition(0)->Enable(true);
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(6)->GetSubMenu()->FindItemByPosition(1)->Enable(true);
  // bookmarks
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Bookmarks")))->FindItemByPosition(0)->Enable(true);
  
  return true;
}


void MyFrameMain::terminate_conn(int which)
{
  if(which == wxNOT_FOUND)
    return;

  VNCConn* c = connections.at(which);
  if(c != 0)
    {
      if(c->isReverse())
	listen_ports.erase(wxAtoi(c->getServerPort()));
    
      connections.erase(connections.begin() + which);

      notebook_connections->DeletePage(which);

      delete c;
    }

  if(connections.size() == 0) // nothing to end
    {
      // "end connection"
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(2)->Enable(false);
      // "screenshot"
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(5)->Enable(false);
      // stats
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(6)->GetSubMenu()->FindItemByPosition(0)->Enable(false);
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(6)->GetSubMenu()->FindItemByPosition(1)->Enable(false);
      // bookmarks
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Bookmarks")))->FindItemByPosition(0)->Enable(false);
    }

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



bool MyFrameMain::loadbookmarks()
{
  wxConfigBase *cfg = wxConfigBase::Get();

  wxArrayString bookmarknames;

  // enumeration variables
  wxString str; 
  long dummy;

  // first, get all bookmark names
  cfg->SetPath(G_BOOKMARKS);
  bool cont = cfg->GetFirstGroup(str, dummy);
  while(cont) 
    {
      bookmarknames.Add(str);
      cont = cfg->GetNextGroup(str, dummy);
    }


  // clean up
  bookmarks.Clear();
  wxMenu* bm_menu = frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Bookmarks")));
  for(int i = bm_menu->GetMenuItemCount()-1; i > 3; --i)
    bm_menu->Destroy(bm_menu->FindItemByPosition(i));
  bm_menu->AppendSeparator();


  // then read in each bookmark value pair
  for(size_t i=0; i < bookmarknames.GetCount(); ++i)
    {
      wxString host, port;

      cfg->SetPath(G_BOOKMARKS + bookmarknames[i]);

      if(!cfg->Read(K_BOOKMARKS_HOST, &host))
	{
	  wxLogError(_("Error reading hostname of bookmark '%s'!"), bookmarknames[i].c_str());
	  cfg->SetPath(wxT("/"));
	  return false;
	}

      if(!cfg->Read(K_BOOKMARKS_PORT, &port))
	{
	  wxLogError(_("Error reading port of bookmark '%s'!"), bookmarknames[i].c_str());
	  cfg->SetPath(wxT("/"));
	  return false;
	}

      // all fine, add it
      bookmarks.Add(host + wxT(":") + port);

      // and add to bookmarks menu
      int id = NewControlId();
      wxString* index_str = new wxString; // pack i into a wxObject, we use wxString here
      *index_str << i;
      bm_menu->Append(id, bookmarknames[i]);
      bm_menu->SetHelpString(id, wxT("Bookmark ") + host + wxT(":") + port);
      Connect(id, wxEVT_COMMAND_MENU_SELECTED, 
	      wxCommandEventHandler(FrameMain::listbox_bookmarks_dclick), (wxObject*)index_str);
    }
  
  cfg->SetPath(wxT("/"));

  list_box_bookmarks->Set(bookmarknames, 0);

  return true;
}





/*
  public members
*/





void MyFrameMain::machine_connect(wxCommandEvent &event)
{
  wxString s = wxGetTextFromUser(_("Enter host to connect to:"),
				 _("Connect to specific host"));
				
  if(s != wxEmptyString)
    spawn_conn(false, s.BeforeFirst(wxT(':')), wxEmptyString, s.AfterFirst(wxT(':')));
}



void MyFrameMain::machine_listen(wxCommandEvent &event)
{
  // find a free port
  set<int>::iterator it;
  bool foundfree = false;
  int port = LISTEN_PORT_OFFSET;
  while(!foundfree)
    {
      if(listen_ports.find(port) == listen_ports.end()) // not in set
	foundfree = true;
      else
	++port;
    }

  listen_ports.insert(port);
  spawn_conn(true, wxEmptyString, wxEmptyString, wxString() << port);
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
      pConfig->Write(K_COMPRESSLEVEL, dialog_settings.getCompressLevel());
      pConfig->Write(K_QUALITY, dialog_settings.getQuality());
      pConfig->Write(K_STATSAUTOSAVE, dialog_settings.getStatsAutosave());
      pConfig->Write(K_LOGSAVETOFILE, dialog_settings.getLogSavetofile());

      VNCConn::doLogfile(dialog_settings.getLogSavetofile());
    }
}


void MyFrameMain::machine_screenshot(wxCommandEvent &event)
{
  if(connections.size())
    {
      VNCConn* c = connections.at(notebook_connections->GetSelection());

      wxRect rect(0, 0, c->getFrameBufferWidth(), c->getFrameBufferHeight());
      if(rect.IsEmpty())
	return;
      wxBitmap screenshot = c->getFrameBufferRegion(rect);
      
      wxString desktopname =  c->getDesktopName();
#ifdef __WIN32__
      // windows doesn't like ':'s
      desktopname.Replace(wxString(wxT(":")), wxString(wxT("-")));
#endif
      wxString filename = wxFileSelector(_("Save screenshot..."), 
					 wxEmptyString,
					 desktopname + wxT("-Screenshot.png"), 
					 wxT(".png"), 
					 _("PNG files|*.png"), 
					 wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
      
      if(!filename.empty())
	{
	  wxBusyCursor busy;
    	  screenshot.SaveFile(filename, wxBITMAP_TYPE_PNG);
	}
    }
}



void MyFrameMain::machine_save_stats_upd(wxCommandEvent &event)
{
  if(connections.size())
    {
      VNCConn* c = connections.at(notebook_connections->GetSelection());
      
      if(c->getUpdateStats().IsEmpty())
	{
	  wxLogMessage(_("Nothing to save!"));
	  return;
	}

      wxString desktopname =  c->getDesktopName();
#ifdef __WIN32__
      // windows doesn't like ':'s
      desktopname.Replace(wxString(wxT(":")), wxString(wxT("-")));
#endif

      wxString filename = wxFileSelector(_("Save framebuffer update statistics..."), 
					 wxEmptyString,
					 desktopname + wxT("-FramebufferUpdate-Stats-") + wxNow() + wxT(".txt"), 
					 wxT(".txt"), 
					 _("TXT files|*.txt"), 
					 wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
      
      if(!filename.empty())
	{
	  wxBusyCursor busy;
	  if(! saveArrayString(const_cast<wxArrayString&>(c->getUpdateStats()), filename))
	    wxLogError(_("Could not save file!"));
	}
    }
}




void MyFrameMain::machine_save_stats_lat(wxCommandEvent &event)
{
  if(connections.size())
    {
      VNCConn* c = connections.at(notebook_connections->GetSelection());

      if(c->getLatencyStats().IsEmpty())
	{
	  wxLogMessage(_("Nothing to save!"));
	  return;
	}

      wxString desktopname =  c->getDesktopName();
#ifdef __WIN32__
      // windows doesn't like ':'s
      desktopname.Replace(wxString(wxT(":")), wxString(wxT("-")));
#endif

      wxString filename = wxFileSelector(_("Save pointer latency statistics..."), 
					 wxEmptyString,
					 desktopname + wxT("-PointerLatency-Stats-") + wxNow() + wxT(".txt"), 
					 wxT(".txt"), 
					 _("TXT files|*.txt"), 
					 wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
      
      if(!filename.empty())
	{
	  wxBusyCursor busy;
	  if(! saveArrayString(const_cast<wxArrayString&>(c->getLatencyStats()), filename))
	    wxLogError(_("Could not save file!"));
	}
    }
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

  if(show_stats)
    stats_timer.Start(STATS_TIMER_INTERVAL);
  else
    stats_timer.Stop();

  text_ctrl_fps->Clear();
  text_ctrl_latency->Clear();

  // for now, toggle all VNCConn instances
  for(size_t i=0; i < connections.size(); ++i)
    connections.at(i)->doStats(show_stats);

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





void MyFrameMain::bookmarks_add(wxCommandEvent &event)
{
  wxString name = wxGetTextFromUser(_("Enter bookmark name:"),
				    _("Saving bookmark"));
				
  if(name != wxEmptyString)
    {
      VNCConn* c = connections.at(notebook_connections->GetSelection());
      wxConfigBase *cfg = wxConfigBase::Get();
      
      if(cfg->Exists(G_BOOKMARKS + name))
	{
	  wxLogError(_("A bookmark with this name already exists!"));
	  return;
	}

      cfg->SetPath(G_BOOKMARKS + name);

      cfg->Write(K_BOOKMARKS_HOST, c->getServerName());
      cfg->Write(K_BOOKMARKS_PORT, c->getServerPort());

      //reset path
      cfg->SetPath(wxT("/"));

      // and load into listbox
      loadbookmarks();
    }
}



void MyFrameMain::bookmarks_edit(wxCommandEvent &event)
{
  wxString sel = list_box_bookmarks->GetStringSelection();

  if(sel.IsEmpty())
    {
      wxLogError(_("No bookmark selected!"));
      return;
    }

  wxString newname = wxGetTextFromUser(_("New bookmark name:"),
				       _("Edit bookmark")); 
  
  if(newname.IsEmpty())
    return;
    
  wxConfigBase *cfg = wxConfigBase::Get();

  cfg->SetPath(G_BOOKMARKS);
  cfg->RenameGroup(sel, newname);
  //reset path
  cfg->SetPath(wxT("/"));
  
  // and load into listbox
  loadbookmarks();
}



void MyFrameMain::bookmarks_delete(wxCommandEvent &event)
{
  wxString sel = list_box_bookmarks->GetStringSelection();
  
  if(sel.IsEmpty())
    {
      wxLogError(_("No bookmark selected!"));
      return;
    }

  wxConfigBase *cfg = wxConfigBase::Get();
  if(!cfg->DeleteGroup(G_BOOKMARKS + sel))
    wxLogError(_("No bookmark with this name!"));

  // and re-read
  loadbookmarks();
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
  wxLogMessage(_("If there are VNC servers advertising themselves via ZeroConf, you can select a host in the 'Available VNC Servers' list. Otherwise, use the 'Connect' button or menu item."));
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
  spawn_conn(false, services_hostname, services_addr, services_port);
} 
 


void MyFrameMain::listbox_bookmarks_select(wxCommandEvent &event)
{
  int sel = event.GetInt();

  if(sel < 0) //nothing selected
    {
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Bookmarks")))->FindItemByPosition(2)->Enable(false);
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Bookmarks")))->FindItemByPosition(3)->Enable(false);
      
      return;
    }
  else
    {
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Bookmarks")))->FindItemByPosition(2)->Enable(true);
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Bookmarks")))->FindItemByPosition(3)->Enable(true);
     
      wxLogStatus(_("Bookmark ") + bookmarks[sel]);
    }
}


void MyFrameMain::listbox_bookmarks_dclick(wxCommandEvent &event)
{
  int sel = event.GetInt();

  if(sel < 0) // nothing selected
    return;

  // this gets set by Connect() in loadbookmarks(), in this case sel is always 0
  if(event.m_callbackUserData)
    sel = wxAtoi(*(wxString*)event.m_callbackUserData);
      
  spawn_conn(false, bookmarks[sel].BeforeFirst(wxT(':')), wxEmptyString, bookmarks[sel].AfterFirst(wxT(':')));
}


bool MyFrameMain::cmdline_connect(wxString& hostarg)
{
  return spawn_conn(false, hostarg.BeforeFirst(wxT(':')), wxEmptyString, hostarg.AfterFirst(wxT(':')));
}
