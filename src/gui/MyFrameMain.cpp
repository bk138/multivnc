
#include <fstream>
#include <wx/aboutdlg.h>
#include <wx/socket.h>
#include <wx/clipbrd.h>
#include <wx/imaglist.h>

#include "res/about.png.h"
#include "res/unicast.png.h"
#include "res/multicast.png.h"

#include "MyFrameMain.h"
#include "MyDialogSettings.h"
#include "../dfltcfg.h"
#include "../MultiVNCApp.h"


using namespace std;


// map recv of custom events to handler methods
BEGIN_EVENT_TABLE(MyFrameMain, FrameMain)
  EVT_COMMAND (wxID_ANY, MyFrameLogCloseNOTIFY, MyFrameMain::onMyFrameLogCloseNotify)
  EVT_COMMAND (wxID_ANY, wxServDiscNOTIFY, MyFrameMain::onSDNotify)
  EVT_VNCCONNUPDATENOTIFY (wxID_ANY, MyFrameMain::onVNCConnUpdateNotify)
  EVT_COMMAND (wxID_ANY, VNCConnUniMultiChangedNOTIFY, MyFrameMain::onVNCConnUniMultiChangedNotify)
  EVT_COMMAND (wxID_ANY, VNCConnFBResizeNOTIFY, MyFrameMain::onVNCConnFBResizeNotify)
  EVT_COMMAND (wxID_ANY, VNCConnCuttextNOTIFY, MyFrameMain::onVNCConnCuttextNotify)
  EVT_COMMAND (wxID_ANY, VNCConnBellNOTIFY, MyFrameMain::onVNCConnBellNotify)
  EVT_COMMAND (wxID_ANY, VNCConnDisconnectNOTIFY, MyFrameMain::onVNCConnDisconnectNotify)
  EVT_COMMAND (wxID_ANY, VNCConnIncomingConnectionNOTIFY, MyFrameMain::onVNCConnIncomingConnectionNotify)
  EVT_END_PROCESS (ID_WINDOWSHARE_PROC_END, MyFrameMain::onWindowshareTerminate)
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
  pConfig->Read(K_SHOWSEAMLESS, &show_seamless, V_SHOWSEAMLESS);
  pConfig->Read(K_SIZE_X, &x, V_SIZE_X);
  pConfig->Read(K_SIZE_Y, &y, V_SIZE_Y);

  bool do_log;
  pConfig->Read(K_LOGSAVETOFILE, &do_log, V_LOGSAVETOFILE);
  VNCConn::doLogfile(do_log);

  // windowshare template
  windowshare_cmd_template = pConfig->Read(K_WINDOWSHARE, V_DFLTWINDOWSHARE);


  // window size
  show_fullscreen = false;
  SetMinSize(wxSize(640, 480));
  splitwin_main->SetMinimumPaneSize(160);
  splitwin_left->SetMinimumPaneSize(250);
  SetSize(x, y);

  // assign image list to notebook_connections
  notebook_connections->AssignImageList(new wxImageList(24, 24));
  notebook_connections->GetImageList()->Add(bitmapFromMem(unicast_png));
  notebook_connections->GetImageList()->Add(bitmapFromMem(multicast_png));


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
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(6)->GetSubMenu()->FindItemByPosition(2)->Enable(false);
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(6)->GetSubMenu()->FindItemByPosition(3)->Enable(false);
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(6)->GetSubMenu()->FindItemByPosition(4)->Enable(false);
  // bookmarks
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Bookmarks")))->FindItemByPosition(0)->Enable(false);
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Bookmarks")))->FindItemByPosition(2)->Enable(false);
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Bookmarks")))->FindItemByPosition(3)->Enable(false);
  // window sharing
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Window Sharing")))->FindItemByPosition(0)->Enable(false);
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Window Sharing")))->FindItemByPosition(1)->Enable(false);

  if(show_toolbar)
    {
      GetToolBar()->EnableTool(wxID_STOP, false); // disconnect
      GetToolBar()->EnableTool(wxID_SAVE, false); // screenshot
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->FindItemByPosition(0)->Check();
    }
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
    frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->FindItemByPosition(3)->Check();
      
  switch(show_seamless)
    {
    case EDGE_NORTH:
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->
	FindItemByPosition(4)->GetSubMenu()->FindItemByPosition(0)->Check();
      break;
    case EDGE_EAST:
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->
	FindItemByPosition(4)->GetSubMenu()->FindItemByPosition(1)->Check();
      break;
    case EDGE_WEST:
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->
	FindItemByPosition(4)->GetSubMenu()->FindItemByPosition(2)->Check();
      break;
    case EDGE_SOUTH:
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->
	FindItemByPosition(4)->GetSubMenu()->FindItemByPosition(3)->Check();
      break;
    default:
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->
	FindItemByPosition(4)->GetSubMenu()->FindItemByPosition(4)->Check();
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

  // this has to be from end to start in order for stats autosave to assign right connection numbers!
  for(int i = connections.size()-1; i >= 0; --i)
    terminate_conn(i);

  delete servscan;


  // since we use the userData parameter in Connect() in loadbookmarks()
  // in a nonstandard way, we have to have this workaround here, otherwise
  // the library would segfault while trying to delete the m_callbackUserData 
  // member of m_dynamicEvents
  if (m_dynamicEvents)
    for ( wxList::iterator it = m_dynamicEvents->begin(),
	    end = m_dynamicEvents->end();
	  it != end;
	  ++it )
      {
#if WXWIN_COMPATIBILITY_EVENT_TYPES
	wxEventTableEntry *entry = (wxEventTableEntry*)*it;
#else 
	wxDynamicEventTableEntry *entry = (wxDynamicEventTableEntry*)*it;
#endif 
	entry->m_callbackUserData = 0;
      }
}





/*
  private members
*/


// handlers

void MyFrameMain::onMyFrameLogCloseNotify(wxCommandEvent& event)
{
  logwindow = 0;
}




void MyFrameMain::onVNCConnUpdateNotify(VNCConnUpdateNotifyEvent& event)
{
  VNCConn* sending_conn = static_cast<VNCConn*>(event.GetEventObject());

  // only draw something for the currently selected connection
  int sel;
  if((sel = notebook_connections->GetSelection()) != -1) 
    {
      VNCConn* selected_conn = connections.at(sel).conn;
      if(selected_conn == sending_conn)
	wxPostEvent((wxEvtHandler*)connections.at(sel).viewerwindow, event);
    }
}



void MyFrameMain::onVNCConnUniMultiChangedNotify(wxCommandEvent& event)
{
  // get sender
  VNCConn* c = static_cast<VNCConn*>(event.GetEventObject());

  // find index of this connection
  vector<ConnBlob>::iterator it = connections.begin();
  size_t index = 0;
  while(it != connections.end() && it->conn != c)
    {
      ++it;
      ++index;
    }

  if(index < connections.size()) // found
    {
      // update icon
      if(c->isMulticast())
	{
	  wxLogStatus( _("Connection to %s is now multicast."), c->getServerHost().c_str());
	  notebook_connections->SetPageImage(index, 1);
	}
      else
	{
	  wxLogStatus( _("Connection to %s is now unicast."), c->getServerHost().c_str());
	  notebook_connections->SetPageImage(index, 0);
	}
    }
}



void MyFrameMain::onVNCConnFBResizeNotify(wxCommandEvent& event)
{
  // get sender
  VNCConn* c = static_cast<VNCConn*>(event.GetEventObject());

  // find index of this connection
  vector<ConnBlob>::iterator it = connections.begin();
  size_t index = 0;
  while(it != connections.end() && it->conn != c)
    {
      ++it;
      ++index;
    }

  if(index < connections.size()) // found
    connections.at(index).viewerwindow->adjustCanvasSize();
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



void MyFrameMain::onVNCConnBellNotify(wxCommandEvent& event)
{ 
  wxBell();
}





void MyFrameMain::onVNCConnDisconnectNotify(wxCommandEvent& event)
{
  // get sender
  VNCConn* c = static_cast<VNCConn*>(event.GetEventObject());

  wxLogStatus( _("Connection to %s:%s terminated."), c->getServerHost().c_str(), c->getServerPort().c_str());
 
  wxArrayString log = VNCConn::getLog();
  // show last 3 log strings
  for(size_t i = log.GetCount() >= 3 ? log.GetCount()-3 : 0; i < log.GetCount(); ++i)
    wxLogMessage(log[i]);
  wxLogMessage( _("Connection to %s:%s terminated."), c->getServerHost().c_str(), c->getServerPort().c_str() );
    
  
  // find index of this connection
  vector<ConnBlob>::iterator it = connections.begin();
  size_t index = 0;
  while(it != connections.end() && it->conn != c)
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
  wxString encodings;
  bool enc_enabled;
  wxConfigBase *pConfig = wxConfigBase::Get();
  pConfig->Read(K_COMPRESSLEVEL, &compresslevel, V_COMPRESSLEVEL);
  pConfig->Read(K_QUALITY, &quality, V_QUALITY);

  pConfig->Read(K_ENC_TIGHT, &enc_enabled, V_ENC_TIGHT);
  if(enc_enabled) 
    encodings += wxT(" tight");
  pConfig->Read(K_ENC_ULTRA, &enc_enabled, V_ENC_ULTRA);
  if(enc_enabled) 
    encodings += wxT(" ultra");
  pConfig->Read(K_ENC_ZLIBHEX, &enc_enabled, V_ENC_ZLIBHEX);
  if(enc_enabled) 
    encodings += wxT(" zlibhex");
  pConfig->Read(K_ENC_ZLIB, &enc_enabled, V_ENC_ZLIB);
  if(enc_enabled) 
    encodings += wxT(" zlib");
  pConfig->Read(K_ENC_ZYWRLE, &enc_enabled, V_ENC_ZYWRLE);
  if(enc_enabled) 
    encodings += wxT(" zywrle");
  pConfig->Read(K_ENC_ZRLE, &enc_enabled, V_ENC_ZRLE);
  if(enc_enabled) 
    encodings += wxT(" zrle");
  pConfig->Read(K_ENC_CORRE, &enc_enabled, V_ENC_CORRE);
  if(enc_enabled) 
    encodings += wxT(" corre");
  pConfig->Read(K_ENC_RRE, &enc_enabled, V_ENC_RRE);
  if(enc_enabled) 
    encodings += wxT(" rre");
  pConfig->Read(K_ENC_HEXTILE, &enc_enabled, V_ENC_HEXTILE);
  if(enc_enabled) 
    encodings += wxT(" hextile");
  pConfig->Read(K_ENC_COPYRECT, &enc_enabled, V_ENC_COPYRECT);
  if(enc_enabled) 
    encodings += wxT(" copyrect");
  // chomp leading space
  encodings = encodings.AfterFirst(' '); 

  
  if(!c->Init(wxEmptyString, encodings, compresslevel, quality))
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
      vector<ConnBlob>::iterator it = connections.begin();
      size_t index = 0;
      while(it != connections.end() && it->conn != c)
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




void MyFrameMain::onWindowshareTerminate(wxProcessEvent& event)
{
  int status = event.GetExitCode();
  int pid = event.GetPid();
  wxLogDebug(wxT("onWindowshareTerminate() called for %d with exit code %d."), pid, status);

  ConnBlob* cb;
  // find index of this connection
  vector<ConnBlob>::iterator it = connections.begin();
  size_t index = 0;
  while(it != connections.end() && it->windowshare_proc_pid != pid)
    {
      ++it;
      ++index;
    }

  //found?
  if(index < connections.size())
    cb = &*it;
  else
    {
      wxLogError(_("Window share helper exited without an associated connection. That should not happen."));
      return;
    }

  if(status == 0) 
    {
      wxString msg = _("Window sharing with ") +
        cb->conn->getDesktopName() +
	_(" stopped. Either the other side does not support receiving windows or the window was closed there.");
      wxLogMessage(msg);
      SetStatusText(msg);
    }
  else
    if(status == -1 || status == 1)
       SetStatusText( _("Window sharing stopped. Shared window was closed."));
    else
      {
	SetStatusText(_("Could not run window share helper."));
	wxLogError(_("Could not run window share helper."));
      }
   
  delete cb->windowshare_proc;
  cb->windowshare_proc = 0;
  cb->windowshare_proc_pid = 0;
  
  // the window sharer of the currently selected session terminated,
  // so update menu
  if(notebook_connections->GetSelection() == (int)index)
    {  
      // this is "share window"
      frame_main_menubar->GetMenu(frame_main_menubar->
				  FindMenu(wxT("Window Sharing")))->FindItemByPosition(0)->Enable(true);
      // "stop window share"
      frame_main_menubar->GetMenu(frame_main_menubar->
				  FindMenu(wxT("Window Sharing")))->FindItemByPosition(1)->Enable(false);
    }
}



char* MyFrameMain::getpasswd(rfbClient* client)
{
  wxString s = wxGetPasswordFromUser(_("Enter password:"),
				     _("Password required!"));
  return strdup(s.char_str());
}



bool MyFrameMain::saveStats(VNCConn* c, int conn_index, const wxArrayString& stats, wxString desc, bool autosave)
{
  if(stats.IsEmpty())
    {
      if(!autosave)
	wxLogMessage(_("Nothing to save!"));
      return true;
    }
  
  wxString header = wxGetHostName() + wxT(" to ") + c->getDesktopName() + wxString::Format(wxT("(%i)"), conn_index) +
    wxT(" ") + desc + wxT(" stats on ") + wxNow();
  
  wxString filename = header + wxT(".csv");
#ifdef __WIN32__
  // windows doesn't like ':'s
  filename.Replace(wxString(wxT(":")), wxString(wxT("-")));
#endif

  if(!autosave)
    filename = wxFileSelector(_("Saving ") + desc +_(" statistics..."), 
			      wxEmptyString,
			      filename, 
			      wxT(".txt"), 
			      _("CSV files|*.csv"), 
			      wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
 
  wxLogDebug(wxT("About to save stats to ") + filename);
      
  if(!filename.empty())
    {
      wxBusyCursor busy;

      ofstream ostream(filename.char_str());
      if(! ostream)
	{
	  wxLogError(_("Could not save file!"));
	  return false;
	}
      // write header
      ostream << header.char_str() << endl;
      // and entries
      for(size_t i=0; i < stats.GetCount(); ++i)
	ostream << stats[i].char_str() << endl;
    }

  return true;
}





// connection initiation and shutdown
bool MyFrameMain::spawn_conn(bool listen, wxString host, wxString port)
{
  wxBusyCursor busy;

  // port can also be something like 'vnc', so look it up...
  wxIPV4address host_addr;
  if(host_addr.Service(port))
    port = wxString() << host_addr.Service();
  else
    port = wxT("5900");
  

  // get connection settings
  int compresslevel, quality, multicast_socketrecvbuf, multicast_recvbuf, fastrequest_interval;
  bool multicast, fastrequest, qos_ef, enc_enabled;
  wxString encodings;
  wxConfigBase *pConfig = wxConfigBase::Get();
  pConfig->Read(K_MULTICAST, &multicast, V_MULTICAST);
  pConfig->Read(K_MULTICASTSOCKETRECVBUF, &multicast_socketrecvbuf, V_MULTICASTSOCKETRECVBUF);
  pConfig->Read(K_MULTICASTRECVBUF, &multicast_recvbuf, V_MULTICASTRECVBUF);
  pConfig->Read(K_FASTREQUEST, &fastrequest, V_FASTREQUEST);
  pConfig->Read(K_FASTREQUESTINTERVAL, &fastrequest_interval, V_FASTREQUESTINTERVAL);
  pConfig->Read(K_QOS_EF, &qos_ef, V_QOS_EF);

  pConfig->Read(K_ENC_TIGHT, &enc_enabled, V_ENC_TIGHT);
  if(enc_enabled) 
    encodings += wxT(" tight");
  pConfig->Read(K_ENC_ULTRA, &enc_enabled, V_ENC_ULTRA);
  if(enc_enabled) 
    encodings += wxT(" ultra");
  pConfig->Read(K_ENC_ZLIBHEX, &enc_enabled, V_ENC_ZLIBHEX);
  if(enc_enabled) 
    encodings += wxT(" zlibhex");
  pConfig->Read(K_ENC_ZLIB, &enc_enabled, V_ENC_ZLIB);
  if(enc_enabled) 
    encodings += wxT(" zlib");
  pConfig->Read(K_ENC_ZYWRLE, &enc_enabled, V_ENC_ZYWRLE);
  if(enc_enabled) 
    encodings += wxT(" zywrle");
  pConfig->Read(K_ENC_ZRLE, &enc_enabled, V_ENC_ZRLE);
  if(enc_enabled) 
    encodings += wxT(" zrle");
  pConfig->Read(K_ENC_CORRE, &enc_enabled, V_ENC_CORRE);
  if(enc_enabled) 
    encodings += wxT(" corre");
  pConfig->Read(K_ENC_RRE, &enc_enabled, V_ENC_RRE);
  if(enc_enabled) 
    encodings += wxT(" rre");
  pConfig->Read(K_ENC_HEXTILE, &enc_enabled, V_ENC_HEXTILE);
  if(enc_enabled) 
    encodings += wxT(" hextile");
  pConfig->Read(K_ENC_COPYRECT, &enc_enabled, V_ENC_COPYRECT);
  if(enc_enabled) 
    encodings += wxT(" copyrect");
  // chomp leading space
  encodings = encodings.AfterFirst(' '); 

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
      wxLogStatus(_("Connecting to ") + host + _T(":") + port + wxT(" ..."));
      if(!c->Init(host + wxT(":") + port, encodings, compresslevel, quality, multicast, multicast_socketrecvbuf, multicast_recvbuf))
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
  
  if(show_stats)
    c->doStats(true);

  ViewerWindow* win = new ViewerWindow(notebook_connections, c);
  win->showStats(show_stats);

  VNCSeamlessConnector* sc = 0;
  if(show_seamless != EDGE_NONE)
    sc = new VNCSeamlessConnector(this, c, show_seamless);

  ConnBlob cb;
  cb.conn = c;
  cb.viewerwindow = win;
  cb.seamlessconnector = sc;  
  cb.windowshare_proc = 0;
  cb.windowshare_proc_pid = 0;

  connections.push_back(cb);

  if(listen)
    notebook_connections->AddPage(win, _("Listening on port ") + port, true);    
  else
    notebook_connections->AddPage(win, c->getDesktopName() + wxT(" (") + c->getServerHost() + wxT(")") , true);

  if(c->isMulticast())
    notebook_connections->SetPageImage(notebook_connections->GetSelection(), 1);
  else
    notebook_connections->SetPageImage(notebook_connections->GetSelection(), 0);

  if(fastrequest)
    c->setFastRequest(fastrequest_interval);

  if(qos_ef)
    c->setDSCP(184); // 184 == 0xb8 == expedited forwarding


  // "end connection"
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(2)->Enable(true);
  // "screenshot"
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(5)->Enable(true);
  // stats
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(6)->GetSubMenu()->FindItemByPosition(0)->Enable(true);
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(6)->GetSubMenu()->FindItemByPosition(1)->Enable(true);
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(6)->GetSubMenu()->FindItemByPosition(2)->Enable(true);
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(6)->GetSubMenu()->FindItemByPosition(3)->Enable(true);
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(6)->GetSubMenu()->FindItemByPosition(4)->Enable(true);
  // bookmarks
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Bookmarks")))->FindItemByPosition(0)->Enable(true);
  // window sharing
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Window Sharing")))->FindItemByPosition(0)->Enable(true);
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Window Sharing")))->FindItemByPosition(1)->Enable(false);

  if(GetToolBar())
    {
      GetToolBar()->EnableTool(wxID_STOP, true); // disconnect
      GetToolBar()->EnableTool(wxID_SAVE, true); // screenshot
    }
  
  return true;
}


void MyFrameMain::terminate_conn(int which)
{
  if(which == wxNOT_FOUND)
    return;

  ConnBlob* cb = &connections.at(which);

  wxConfigBase *pConfig = wxConfigBase::Get();
  bool autosave_stats;
  pConfig->Read(K_STATSAUTOSAVE, &autosave_stats, V_STATSAUTOSAVE);  
  if(autosave_stats)
    {
      VNCConn* c = cb->conn;

      // find index of this connection
      vector<ConnBlob>::iterator it = connections.begin();
      size_t index = 0;
      while(it != connections.end() && it->conn != c)
	{
	  ++it;
	  ++index;
	}

      if(index < connections.size()) // found
	{
	  if(!saveStats(c, index, c->getUpdRawByteStats(), _("frame buffer update raw byte"), true))
	    wxLogError(_("Could not autosave framebuffer update raw byte statistics!"));   
	  if(!saveStats(c, index, c->getUpdCountStats(), _("frame buffer update count"), true))
	    wxLogError(_("Could not autosave framebuffer update count statistics!"));
	  if(!saveStats(c, index, c->getLatencyStats(), _("latency"), true))
	    wxLogError(_("Could not autosave latency statistics!"));    
	  if(!saveStats(c, index, c->getMCLossRatioStats(), _("multicast loss ratio"),true))
	    wxLogError(_("Could not autosave multicast loss ratio statistics!"));
	  if(!saveStats(c, index, c->getMCBufStats(), _("multicast receive buffer"),true))
	    wxLogError(_("Could not autosave multicast receive buffer statistics!"));
	}
    }
 

  if(cb->conn->isReverse())
    listen_ports.erase(wxAtoi(cb->conn->getServerPort()));
  // this deletes the ViewerWindow plus canvas   
  notebook_connections->DeletePage(which);
  // this deletes the seamless connector
  if(cb->seamlessconnector) 
    delete cb->seamlessconnector;
  // this deletes the VNCConn
  delete cb->conn;
  // this deletes the windowsharer, if there's any
  if(cb->windowshare_proc)
    {
      wxLogDebug(wxT("terminate_conn(): trying to kill %d."), cb->windowshare_proc_pid);
      if(!wxProcess::Exists(cb->windowshare_proc_pid))
	{
	  wxLogDebug(wxT("terminate_conn(): window sharing helper PID does not exist!"));
	}
      else
	{
	  // avoid callback call, now obj pointed to by windowshare_proc deletes itself!
	  cb->windowshare_proc->Detach(); 
	  if(wxKill(cb->windowshare_proc_pid, wxSIGTERM, NULL, wxKILL_CHILDREN) == 0)
	    wxLogDebug(wxT("terminate_conn(): successfully killed %d."), cb->windowshare_proc_pid);
	  else
	    wxLogDebug(wxT("terminate_conn(): could not kill %d."), cb->windowshare_proc_pid);
	}
    }
  // erase the ConnBlob
  connections.erase(connections.begin() + which);

  if(connections.size() == 0) // nothing to end
    {
      // "end connection"
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(2)->Enable(false);
      // "screenshot"
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(5)->Enable(false);
      // stats
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(6)->GetSubMenu()->FindItemByPosition(0)->Enable(false);
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(6)->GetSubMenu()->FindItemByPosition(1)->Enable(false);
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(6)->GetSubMenu()->FindItemByPosition(2)->Enable(false);
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(6)->GetSubMenu()->FindItemByPosition(3)->Enable(false);
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Machine")))->FindItemByPosition(6)->GetSubMenu()->FindItemByPosition(4)->Enable(false);
      // bookmarks
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Bookmarks")))->FindItemByPosition(0)->Enable(false);
      // window sharing
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Window Sharing")))->FindItemByPosition(0)->Enable(false);
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Window Sharing")))->FindItemByPosition(1)->Enable(false);

      if(GetToolBar())
	{
	  GetToolBar()->EnableTool(wxID_STOP, false); // disconnect
	  GetToolBar()->EnableTool(wxID_SAVE, false); // screenshot
	}
    }

  wxLogStatus( _("Connection terminated."));
}






void MyFrameMain::splitwinlayout()
{
  // setup layout in respect to every possible combination
  
  if(!show_discovered && !show_bookmarks) // 00
    {
      splitwin_main->Unsplit(splitwin_main_pane_1);
    }
 
  if(!show_discovered && show_bookmarks)  // 01
    {
      splitwin_main->SplitVertically(splitwin_main_pane_1, splitwin_main_pane_2);

      splitwin_left->SplitHorizontally(splitwin_left_pane_1, splitwin_left_pane_2);
      splitwin_left->Unsplit(splitwin_left_pane_1);
    }
 
  if(show_discovered && !show_bookmarks) // 10
    {
      splitwin_main->SplitVertically(splitwin_main_pane_1, splitwin_main_pane_2);

      splitwin_left->SplitHorizontally(splitwin_left_pane_1, splitwin_left_pane_2);
      splitwin_left->Unsplit(splitwin_left_pane_2);
    }
 
  if(show_discovered && show_bookmarks) // 11
    {
      splitwin_main->SplitVertically(splitwin_main_pane_1, splitwin_main_pane_2);

      splitwin_left->SplitHorizontally(splitwin_left_pane_1, splitwin_left_pane_2);
    }
 

  // and set proportions
  int w,h;
  GetSize(&w, &h);

  splitwin_main->SetSashPosition(w * 0.1);
  splitwin_left->SetSashPosition(h * 0.4);
  
  // finally if not shown, disable menu items
  if(!show_bookmarks)
    {
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Bookmarks")))->FindItemByPosition(2)->Enable(false);
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Bookmarks")))->FindItemByPosition(3)->Enable(false);
    }  
  else
    {
      if(list_box_bookmarks->GetSelection() >= 0)
	{
	  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Bookmarks")))->FindItemByPosition(2)->Enable(true);
	  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Bookmarks")))->FindItemByPosition(3)->Enable(true);
	}
    }
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
    spawn_conn(false, s.BeforeFirst(wxT(':')), s.AfterFirst(wxT(':')));
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
  spawn_conn(true, wxEmptyString, wxString() << port);
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
      pConfig->Write(K_STATSAUTOSAVE, dialog_settings.getStatsAutosave());
      pConfig->Write(K_LOGSAVETOFILE, dialog_settings.getLogSavetofile());
      pConfig->Write(K_MULTICAST, dialog_settings.getDoMulticast());
      pConfig->Write(K_MULTICASTSOCKETRECVBUF, dialog_settings.getMulticastSocketRecvBuf());
      pConfig->Write(K_MULTICASTRECVBUF, dialog_settings.getMulticastRecvBuf());
      pConfig->Write(K_FASTREQUEST, dialog_settings.getDoFastRequest());
      pConfig->Write(K_FASTREQUESTINTERVAL, dialog_settings.getFastRequestInterval());
      pConfig->Write(K_QOS_EF, dialog_settings.getQoS_EF());

      pConfig->Write(K_ENC_COPYRECT, dialog_settings.getEncCopyRect());
      pConfig->Write(K_ENC_HEXTILE, dialog_settings.getEncHextile());
      pConfig->Write(K_ENC_RRE, dialog_settings.getEncRRE());
      pConfig->Write(K_ENC_CORRE, dialog_settings.getEncCoRRE());
      pConfig->Write(K_ENC_ZLIB, dialog_settings.getEncZlib());
      pConfig->Write(K_ENC_ZLIBHEX, dialog_settings.getEncZlibHex());
      pConfig->Write(K_ENC_ZRLE, dialog_settings.getEncZRLE());
      pConfig->Write(K_ENC_ZYWRLE, dialog_settings.getEncZYWRLE());
      pConfig->Write(K_ENC_ULTRA, dialog_settings.getEncUltra());
      pConfig->Write(K_ENC_TIGHT, dialog_settings.getEncTight());
      pConfig->Write(K_COMPRESSLEVEL, dialog_settings.getCompressLevel());
      pConfig->Write(K_QUALITY, dialog_settings.getQuality());

      VNCConn::doLogfile(dialog_settings.getLogSavetofile());
    }
}


void MyFrameMain::machine_screenshot(wxCommandEvent &event)
{
  if(connections.size())
    {
      VNCConn* c = connections.at(notebook_connections->GetSelection()).conn;

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



void MyFrameMain::machine_save_stats_upd_rawbytes(wxCommandEvent &event)
{
  if(connections.size())
    {
      int sel = notebook_connections->GetSelection();
      VNCConn* c = connections.at(sel).conn;
      saveStats(c, sel, c->getUpdRawByteStats(), _("framebuffer update raw byte"), false);
    }
}


void MyFrameMain::machine_save_stats_upd_count(wxCommandEvent &event)
{
 if(connections.size())
    {
      int sel = notebook_connections->GetSelection();
      VNCConn* c = connections.at(sel).conn;
      saveStats(c, sel, c->getUpdCountStats(), _("framebuffer update count"), false);
    }
}



void MyFrameMain::machine_save_stats_latencies(wxCommandEvent &event)
{
 if(connections.size())
    {
      int sel = notebook_connections->GetSelection();
      VNCConn* c = connections.at(sel).conn;
      saveStats(c, sel, c->getLatencyStats(), _("latency"), false);
    }
}


void MyFrameMain::machine_save_stats_lossratio(wxCommandEvent &event)
{
  if(connections.size())
    {
      int sel = notebook_connections->GetSelection();
      VNCConn* c = connections.at(sel).conn;
      saveStats(c, sel, c->getMCLossRatioStats(), _("multicast loss ratio"), false);
    }
}


void MyFrameMain::machine_save_stats_recvbuf(wxCommandEvent &event)
{
  if(connections.size())
    {
      int sel = notebook_connections->GetSelection();
      VNCConn* c = connections.at(sel).conn;
      saveStats(c, sel, c->getMCBufStats(), _("multicast receive buffer"), false);
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
      frame_main_toolbar = new wxToolBar(this, -1, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL|wxTB_DOCKABLE|wxTB_3DBUTTONS|wxTB_TEXT);
      SetToolBar(frame_main_toolbar);
      frame_main_toolbar->SetToolBitmapSize(wxSize(24, 24));
      frame_main_toolbar->AddTool(wxID_YES, _("Connect"), (bitmapFromMem(connect_png)), (bitmapFromMem(connect_png)), wxITEM_NORMAL, wxEmptyString, wxEmptyString);
      frame_main_toolbar->AddTool(wxID_REDO, _("Listen"), (bitmapFromMem(listen_png)), (bitmapFromMem(listen_png)), wxITEM_NORMAL, wxEmptyString, wxEmptyString);
      frame_main_toolbar->AddTool(wxID_STOP, _("Disconnect"), (bitmapFromMem(disconnect_png)), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
      frame_main_toolbar->AddSeparator();
      frame_main_toolbar->AddTool(wxID_ZOOM_FIT, _("Fullscreen"), (bitmapFromMem(fullscreen_png)), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
      frame_main_toolbar->AddTool(wxID_SAVE, _("Take Screenshot"), (bitmapFromMem(screenshot_png)), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
      frame_main_toolbar->Realize();
      
      bool enable = connections.size() != 0;
	
      frame_main_toolbar->EnableTool(wxID_STOP, enable); // disconnect
      frame_main_toolbar->EnableTool(wxID_SAVE, enable); // screenshot
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

  // for now, toggle all connections
  for(size_t i=0; i < connections.size(); ++i)
    {
      connections.at(i).conn->doStats(show_stats);
      connections.at(i).viewerwindow->showStats(show_stats);
    }

  wxConfigBase *pConfig = wxConfigBase::Get();
  pConfig->Write(K_SHOWSTATS, show_stats);
}





void MyFrameMain::view_togglefullscreen(wxCommandEvent &event)
{
  show_fullscreen = ! show_fullscreen;

  ShowFullScreen(show_fullscreen, 
		 wxFULLSCREEN_NOTOOLBAR | wxFULLSCREEN_NOSTATUSBAR | wxFULLSCREEN_NOBORDER | wxFULLSCREEN_NOCAPTION);

  if(show_fullscreen)
    frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->FindItemByPosition(6)->Check();
}




void MyFrameMain::view_seamless(wxCommandEvent &event)
{
  if(frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->
     FindItemByPosition(4)->GetSubMenu()->FindItemByPosition(0)->IsChecked())
    show_seamless = EDGE_NORTH;
  else if
    (frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->
     FindItemByPosition(4)->GetSubMenu()->FindItemByPosition(1)->IsChecked())
    show_seamless = EDGE_EAST;
  else if 
    (frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->
     FindItemByPosition(4)->GetSubMenu()->FindItemByPosition(2)->IsChecked())
    show_seamless = EDGE_WEST;
  else if
    (frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->
     FindItemByPosition(4)->GetSubMenu()->FindItemByPosition(3)->IsChecked())
    show_seamless = EDGE_SOUTH;
  else
    show_seamless = EDGE_NONE;

  //change for active connection
  if(connections.size())
    {
      ConnBlob* cbp = &connections.at(notebook_connections->GetSelection());
      if(cbp->seamlessconnector) 
	{
	  delete cbp->seamlessconnector;
	  cbp->seamlessconnector = 0;
	}
      if(show_seamless != EDGE_NONE)
	cbp->seamlessconnector = new VNCSeamlessConnector(this, cbp->conn, show_seamless);
    }
  

  wxConfigBase *pConfig = wxConfigBase::Get();
  pConfig->Write(K_SHOWSEAMLESS, show_seamless);
}





void MyFrameMain::bookmarks_add(wxCommandEvent &event)
{
  VNCConn* c = connections.at(notebook_connections->GetSelection()).conn;
  wxConfigBase *cfg = wxConfigBase::Get();

  if(c->getServerHost().IsEmpty())
	{
	  wxLogError(_("Cannot bookmark a reverse connection!"));
	  return;
	}

  wxString name = wxGetTextFromUser(_("Enter bookmark name:"),
				    _("Saving bookmark"));
				
  if(name != wxEmptyString)
    {
      if(cfg->Exists(G_BOOKMARKS + name))
	{
	  wxLogError(_("A bookmark with this name already exists!"));
	  return;
	}

      cfg->SetPath(G_BOOKMARKS + name);

      cfg->Write(K_BOOKMARKS_HOST, c->getServerHost());
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

  wxString desc = _("\nMultiVNC is a cross-platform Multicast-enabled VNC client.\n");
  desc += _("\nBuilt with ") + (wxString() << wxVERSION_STRING) + wxT("\n");
  desc += _("\nSupported Security Types:\n");
  desc += _("VNC Authentication");
#ifdef LIBVNCSERVER_WITH_CLIENT_TLS
  desc += wxT(", Anonymous TLS, VeNCrypt");
#endif
  desc += _("\n\nSupported Encodings:\n");
  desc += wxT("Raw, RRE, coRRE, CopyRect, Hextile, Ultra");
#ifdef LIBVNCSERVER_HAVE_LIBZ 
  desc += wxT(", UltraZip, Zlib, ZlibHex, ZRLE, ZYWRLE");
#ifdef LIBVNCSERVER_HAVE_LIBJPEG 
  desc += wxT(", Tight");
#endif
#endif

  info.SetIcon(icon);
  info.SetName(wxT("MultiVNC"));
  info.SetVersion(wxT(VERSION));
  info.SetDescription(desc);
  info.SetCopyright(wxT(COPYRIGHT));
  
  wxAboutBox(info); 
}




void MyFrameMain::help_contents(wxCommandEvent &e)
{
  wxLogMessage(_("\
If there are VNC servers advertising themselves via ZeroConf, you can select a host in the\
'Available VNC Servers' list. Otherwise, use the 'Connect' button or menu item.\
\n\nWhen connected, a blue or green icon on the tab label shows if you are running in unicast \
or multicast mode."));
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
  spawn_conn(false, services_addr, services_port);
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
      
  spawn_conn(false, bookmarks[sel].BeforeFirst(wxT(':')), bookmarks[sel].AfterFirst(wxT(':')));
}


void MyFrameMain::notebook_connections_pagechanged(wxNotebookEvent &event)
{
  ConnBlob* cb;
  if(connections.size())
    cb = &connections.at(notebook_connections->GetSelection());
  else
    return;

  bool isSharing = cb->windowshare_proc ? true : false;
  wxLogDebug(wxT("notebook_connections_pagechanged(): VNCConn %p sharing is %d"), cb->conn, isSharing);
  // this is "share window"
  frame_main_menubar->GetMenu(frame_main_menubar->
			      FindMenu(wxT("Window Sharing")))->FindItemByPosition(0)->Enable(!isSharing);
  // this is "stop share window"
  frame_main_menubar->GetMenu(frame_main_menubar->
			      FindMenu(wxT("Window Sharing")))->FindItemByPosition(1)->Enable(isSharing);

  if(cb->seamlessconnector) 
    {
      switch(cb->seamlessconnector->getEdge())
	{
	case EDGE_NORTH:
	  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->
	    FindItemByPosition(4)->GetSubMenu()->FindItemByPosition(0)->Check();
	  break;
	case EDGE_EAST:
	  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->
	    FindItemByPosition(4)->GetSubMenu()->FindItemByPosition(1)->Check();
	  break;
	case EDGE_WEST:
	  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->
	    FindItemByPosition(4)->GetSubMenu()->FindItemByPosition(2)->Check();
	  break;
	case EDGE_SOUTH:
	  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->
	    FindItemByPosition(4)->GetSubMenu()->FindItemByPosition(3)->Check();
	  break;
	default:
	  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->
	    FindItemByPosition(4)->GetSubMenu()->FindItemByPosition(4)->Check();
	}

      cb->seamlessconnector->Raise();
    }
  else // no object, i.e. disabled
    frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("View")))->
      FindItemByPosition(4)->GetSubMenu()->FindItemByPosition(4)->Check();
}



bool MyFrameMain::cmdline_connect(wxString& hostarg)
{
  return spawn_conn(false, hostarg.BeforeFirst(wxT(':')), hostarg.AfterFirst(wxT(':')));
}




void MyFrameMain::windowshare_start(wxCommandEvent &event)
{
  wxBusyCursor busy;

  ConnBlob* cb = 0;
  if(connections.size())
    cb = &connections.at(notebook_connections->GetSelection());
  else
    return;

  // right now x11vnc and WinVNC behave differently :-/
#ifdef __WIN32__
  wxString window = wxGetTextFromUser(_("Enter name of window to share:"), _("Share a Window"));
  if(window == wxEmptyString)
    return;
#else
  if(wxMessageBox(_("A cross-shaped cursor will appear. Use it to select the window you want to share."),
		  _("Share a Window"), wxOK|wxCANCEL)
     == wxCANCEL)
    return;
#endif

  // handle %a and %p
  wxString cmd = windowshare_cmd_template;
  cmd.Replace(wxT("%a"), cb->conn->getServerHost());
  //  cmd.Replace(_T("%p"), port); //unused by now
#ifdef __WIN32__
  cmd.Replace(wxT("%w"), window);
#endif
  
  // terminate old one
  wxCommandEvent unused;
  windowshare_stop(unused);

  // and start new one
  cb->windowshare_proc = new wxProcess(this, ID_WINDOWSHARE_PROC_END);
  cb->windowshare_proc_pid = wxExecute(cmd, wxEXEC_ASYNC|wxEXEC_MAKE_GROUP_LEADER, cb->windowshare_proc);
  wxLogDebug(wxT("windowshare_start() spawned %d."), cb->windowshare_proc_pid);
  
  if(cb->windowshare_proc_pid == 0)
    {
      SetStatusText(_("Window sharing helper execution failed."));
      wxLogError( _("Could not share window, external program execution failed."));

      delete cb->windowshare_proc;
      cb->windowshare_proc = 0; 
      cb->windowshare_proc_pid = 0;
      return;
    }

  SetStatusText(_("Sharing window with ") + cb->conn->getDesktopName());
		
  // this is "share window"
  frame_main_menubar->GetMenu(frame_main_menubar->
			      FindMenu(wxT("Window Sharing")))->FindItemByPosition(0)->Enable(false);
  // this is "stop share window"
  frame_main_menubar->GetMenu(frame_main_menubar->
			      FindMenu(wxT("Window Sharing")))->FindItemByPosition(1)->Enable(true);
}




void MyFrameMain::windowshare_stop(wxCommandEvent &event)
{
  wxBusyCursor busy;

  ConnBlob* cb;
  if(connections.size())
    cb = &connections.at(notebook_connections->GetSelection());
  else
    return;

  if(!cb->windowshare_proc || !cb->windowshare_proc_pid)
    return;

  wxLogDebug(wxT("windowshare_stop(): tries to kill %d."), cb->windowshare_proc_pid);

  if(!wxProcess::Exists(cb->windowshare_proc_pid))
    {
      wxLogDebug(wxT("windowshare_stop(): PID does not exist, exiting!"));
      return;
    }

  // avoid callback call, now obj pointed to by windowshare_proc deletes itself!
  cb->windowshare_proc->Detach(); 

  if(wxKill(cb->windowshare_proc_pid, wxSIGTERM, NULL, wxKILL_CHILDREN) == 0)
    {
      wxLogDebug(wxT("windowshare_stop(): successfully killed %d."), cb->windowshare_proc_pid);
      cb->windowshare_proc = 0; // obj deleted itself because of Detach()!
      cb->windowshare_proc_pid = 0;
      
      wxLogStatus(_("Stopped sharing window with ") + cb->conn->getDesktopName());

      // this is "share window"
      frame_main_menubar->GetMenu(frame_main_menubar->
				  FindMenu(wxT("Window Sharing")))->FindItemByPosition(0)->Enable(true);
      // this is "stop share window"
      frame_main_menubar->GetMenu(frame_main_menubar->
				  FindMenu(wxT("Window Sharing")))->FindItemByPosition(1)->Enable(false);
    }
  else
    wxLogDebug(wxT("windowshare_stop(): Could not kill %d. Not good."), cb->windowshare_proc_pid);
}




