
#include "gui/bitmapFromMem.h"
#include "gui/evtids.h"
#include <fstream>
#include <wx/aboutdlg.h>
#include <wx/socket.h>
#include <wx/clipbrd.h>
#include <wx/imaglist.h>
#include <wx/secretstore.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>

#include "MyFrameMain.h"
#include "MyDialogSettings.h"
#include "DialogLogin.h"
#include "../dfltcfg.h"
#include "../MultiVNCApp.h"


using namespace std;

#ifdef __WXGTK__ // only GTK supports this atm
#define MULTIVNC_GRABKEYBOARD
#endif

#ifndef EVT_FULLSCREEN
// workaround for missing EVT_FULLSCREEN
#define wxFullScreenEventHandler(func) \
    wxEVENT_HANDLER_CAST(wxFullScreenEventFunction, func)
typedef void (wxEvtHandler::*wxFullScreenEventFunction)(wxFullScreenEvent&);
#define EVT_FULLSCREEN(func) wx__DECLARE_EVT0(wxEVT_FULLSCREEN, wxFullScreenEventHandler(func))
#endif

// map recv of custom events to handler methods
BEGIN_EVENT_TABLE(MyFrameMain, FrameMain)
  EVT_COMMAND (wxID_ANY, MyFrameLogCloseNOTIFY, MyFrameMain::onMyFrameLogCloseNotify)
  EVT_COMMAND (wxID_ANY, wxServDiscNOTIFY, MyFrameMain::onSDNotify)
  EVT_COMMAND (wxID_ANY, VNCConnListenNOTIFY, MyFrameMain::onVNCConnListenNotify)
  EVT_COMMAND (wxID_ANY, VNCConnInitNOTIFY, MyFrameMain::onVNCConnInitNotify)
  EVT_COMMAND (wxID_ANY, VNCConnGetPasswordNOTIFY, MyFrameMain::onVNCConnGetPasswordNotify)
  EVT_COMMAND (wxID_ANY, VNCConnGetCredentialsNOTIFY, MyFrameMain::onVNCConnGetCredentialsNotify)
  EVT_VNCCONNUPDATENOTIFY (wxID_ANY, MyFrameMain::onVNCConnUpdateNotify)
  EVT_COMMAND (wxID_ANY, VNCConnUniMultiChangedNOTIFY, MyFrameMain::onVNCConnUniMultiChangedNotify)
  EVT_COMMAND (wxID_ANY, VNCConnReplayFinishedNOTIFY, MyFrameMain::onVNCConnReplayFinishedNotify)
  EVT_COMMAND (wxID_ANY, VNCConnFBResizeNOTIFY, MyFrameMain::onVNCConnFBResizeNotify)
  EVT_COMMAND (wxID_ANY, VNCConnCuttextNOTIFY, MyFrameMain::onVNCConnCuttextNotify)
  EVT_COMMAND (wxID_ANY, VNCConnBellNOTIFY, MyFrameMain::onVNCConnBellNotify)
  EVT_COMMAND (wxID_ANY, VNCConnDisconnectNOTIFY, MyFrameMain::onVNCConnDisconnectNotify)
  EVT_COMMAND (wxID_ANY, VNCConnIncomingConnectionNOTIFY, MyFrameMain::onVNCConnIncomingConnectionNotify)
  EVT_END_PROCESS (ID_WINDOWSHARE_PROC_END, MyFrameMain::onWindowshareTerminate)
  EVT_FULLSCREEN (MyFrameMain::onFullScreenChanged)
  EVT_SYS_COLOUR_CHANGED(MyFrameMain::onSysColourChanged)
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
  bool grab_keyboard;
  // get default config object, created on demand if not exist
  wxConfigBase *pConfig = wxConfigBase::Get();
  pConfig->Read(K_SHOWTOOLBAR, &show_toolbar, V_SHOWTOOLBAR);
  pConfig->Read(K_SHOWDISCOVERED, &show_discovered, V_SHOWDISCOVERED);
  pConfig->Read(K_SHOWBOOKMARKS, &show_bookmarks, V_SHOWBOOKMARKS);
  pConfig->Read(K_SHOWSTATS, &show_stats, V_SHOWSTATS);
  pConfig->Read(K_SHOWSEAMLESS, &show_seamless, V_SHOWSEAMLESS);
  pConfig->Read(K_SHOW1TO1, &show_1to1, V_SHOW1TO1);
  pConfig->Read(K_GRABKEYBOARD, &grab_keyboard, V_GRABKEYBOARD);
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
  EnableFullScreenView();


  // assign image list to notebook_connections
  notebook_connections->AssignImageList(new wxImageList(24, 24));
  notebook_connections->GetImageList()->Add(bitmapBundleFromSVGResource("unicast").GetBitmapFor(this));
  notebook_connections->GetImageList()->Add(bitmapBundleFromSVGResource("multicast").GetBitmapFor(this));


  /*
    setup menu items for the frame
  */
  // "disconnect"
  frame_main_menubar->Enable(wxID_STOP, false);
  // "screenshot"
  frame_main_menubar->Enable(wxID_SAVE, false);
  // stats
  frame_main_menubar->Enable(ID_STATS_SAVE, false);
  // record/replay
  frame_main_menubar->Enable(ID_INPUT_RECORD, false);
  frame_main_menubar->Enable(ID_INPUT_REPLAY, false);
  // bookmarks
  frame_main_menubar->Enable(wxID_ADD, false);
  frame_main_menubar->Enable(wxID_EDIT, false);
  frame_main_menubar->Enable(wxID_DELETE, false);
  // window sharing
  frame_main_menubar->Enable(wxID_UP, false);
  frame_main_menubar->Enable(wxID_CANCEL, false);
#ifdef __WXGTK__
  wxString sessionType, flatpakId;
  wxGetEnv("XDG_SESSION_TYPE", &sessionType);
  wxGetEnv("FLATPAK_ID", &flatpakId);
  // don't show for flatpak and wayland
  if(!flatpakId.IsEmpty() || !sessionType.IsSameAs("x11"))
      frame_main_menubar->Remove(frame_main_menubar->FindMenu(_("Window &Sharing")));
#elif defined __WXMSW__
  // always on
#else
  // always off so far
  frame_main_menubar->Remove(frame_main_menubar->FindMenu(_("Window &Sharing")));
#endif
  // edge connector
  if(!VNCSeamlessConnector::isSupportedByCurrentPlatform())
      frame_main_menubar->FindItem(ID_SEAMLESS)->GetMenu()->Delete(ID_SEAMLESS);

  // toolbar setup
#ifdef MULTIVNC_GRABKEYBOARD
  frame_main_toolbar->ToggleTool(ID_GRABKEYBOARD, grab_keyboard);
#else 
  frame_main_toolbar->DeleteTool(ID_GRABKEYBOARD);
#endif


  if(show_toolbar)
    {
      frame_main_toolbar->EnableTool(wxID_STOP, false); // disconnect
      frame_main_toolbar->EnableTool(wxID_SAVE, false); // screenshot
      frame_main_toolbar->EnableTool(ID_INPUT_REPLAY, false);
      frame_main_toolbar->EnableTool(ID_INPUT_RECORD, false);

      frame_main_menubar->Check(ID_TOOLBAR, true);
    }
  else
    {
      frame_main_toolbar->Show(false);
    }


  splitwinlayout();

  loadbookmarks();

  if(show_discovered)
    frame_main_menubar->Check(ID_DISCOVERED, true);
  if(show_bookmarks)
    frame_main_menubar->Check(ID_BOOKMARKS, true);
  if(show_stats)
    frame_main_menubar->Check(ID_STATISTICS, true);
      
  switch(show_seamless)
    {
    case EDGE_NORTH:
      frame_main_menubar->Check(ID_SEAMLESS_NORTH, true);
      break;
    case EDGE_EAST:
      frame_main_menubar->Check(ID_SEAMLESS_EAST, true);
      break;
    case EDGE_WEST:
      frame_main_menubar->Check(ID_SEAMLESS_WEST, true);
      break;
    case EDGE_SOUTH:
      frame_main_menubar->Check(ID_SEAMLESS_SOUTH, true);
      break;
    default:
      frame_main_menubar->Check(ID_SEAMLESS_DISABLED, true);
    }

  if(show_1to1) {
      frame_main_menubar->Check(ID_ONE_TO_ONE, true);
      GetToolBar()->ToggleTool(ID_ONE_TO_ONE, true);
  }

  // setup clipboard
#ifdef __WXGTK__
  // always use middle mouse button paste
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
#ifdef MULTIVNC_GRABKEYBOARD
  pConfig->Write(K_GRABKEYBOARD, frame_main_toolbar->GetToolState(ID_GRABKEYBOARD));
#endif

  // this has to be from end to start in order for stats autosave to assign right connection numbers!
  for(int i = connections.size()-1; i >= 0; --i)
    terminate_conn(i);

  delete servscan;


  // since we use the userData parameter in Connect() in loadbookmarks()
  // in a nonstandard way, we have to have this workaround here, otherwise
  // the library would segfault while trying to delete the m_callbackUserData 
  // member of m_dynamicEvents
  if (m_dynamicEvents)
    for ( wxVector<wxDynamicEventTableEntry*>::iterator it = m_dynamicEvents->begin(),
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


void MyFrameMain::onVNCConnListenNotify(wxCommandEvent& event)
{
    VNCConn* c = static_cast<VNCConn*>(event.GetEventObject());
    if (event.GetInt() == 0) {
	setup_conn(c);
    } else {
	// listen error
	wxLogError(c->getErr());
	delete c;
    }
}

void MyFrameMain::onVNCConnInitNotify(wxCommandEvent& event)
{
    wxEndBusyCursor();

    VNCConn* c = static_cast<VNCConn*>(event.GetEventObject());

    if (event.GetInt() == 0) {
	setup_conn(c);
    } else {
	// error. only show error if this was not a auth case with empty password, i.e. a canceled one
	if (c->getRequireAuth()
#if wxUSE_SECRETSTORE
	    && !c->getPassword().IsOk()) {
#else
	    && c->getPassword().IsEmpty()) {
#endif
	    wxLogStatus(_("Authentication canceled."));
        } else {
	    wxLogStatus(_("Connection failed."));
            wxArrayString log = VNCConn::getLog();
            // show last 3 log strings
            for (size_t i = log.GetCount() >= 3 ? log.GetCount() - 3 : 0;
                 i < log.GetCount(); ++i)
                wxLogMessage(log[i]);
            wxLogError(c->getErr());
        }

	// find out if we already setup this this connection.
	// this happens if it was a listening one that failed it's Init()
	vector<ConnBlob>::iterator it = connections.begin();
	size_t index = 0;
	while(it != connections.end() && it->conn != c)
	    {
		++it;
		++index;
	    }
	if (index < connections.size()) {
	    // found it. terminate!
	    terminate_conn(index);
        } else {
	    // not yet setup in UI, simply delete
	    delete c;
        }
    }
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
	  wxArrayString log = VNCConn::getLog();
	  // show last 3 log strings
	  for(size_t i = log.GetCount() >= 3 ? log.GetCount()-3 : 0; i < log.GetCount(); ++i)
	    wxLogMessage(log[i]);
	  wxLogMessage( _("Connection to %s switched to unicast."), c->getServerHost().c_str());

	  wxLogStatus( _("Connection to %s is now unicast."), c->getServerHost().c_str());
	  notebook_connections->SetPageImage(index, 0);
	}
    }
}




void MyFrameMain::onVNCConnReplayFinishedNotify(wxCommandEvent& event)
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
      wxString prefix = wxSystemSettings::GetAppearance().IsDark() ? "dark" : "light";
      frame_main_toolbar->SetToolNormalBitmap(ID_INPUT_REPLAY, bitmapBundleFromSVGResource(prefix + "/" + "replay"));
      frame_main_toolbar->FindById(ID_INPUT_REPLAY)->SetLabel(_("Replay Input"));
      frame_main_menubar->SetLabel(ID_INPUT_REPLAY, _("Replay Input"));

      // re-enable record buttons
      GetToolBar()->EnableTool(ID_INPUT_RECORD, true);
      frame_main_menubar->Enable(ID_INPUT_RECORD, true);

      wxLogMessage( _("Replay finished!"));
      wxLogStatus(_("Replay finished!"));
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

  if (!c->getServerHost().IsEmpty()) {
      wxLogStatus(_("Connection to %s:%s terminated."), c->getServerHost().c_str(), c->getServerPort().c_str());
  } else {
      wxLogStatus(_("Reverse connection terminated."));
  }

  wxArrayString log = VNCConn::getLog();
  // show last 3 log strings
  for(size_t i = log.GetCount() >= 3 ? log.GetCount()-3 : 0; i < log.GetCount(); ++i)
    wxLogMessage(log[i]);
  if (!c->getServerHost().IsEmpty()) {
      wxLogMessage(_("Connection to %s:%s terminated."), c->getServerHost().c_str(), c->getServerPort().c_str());
  } else {
      wxLogMessage(_("Reverse connection terminated."));
  }

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

  
  c->Init(wxEmptyString, wxEmptyString,
#if wxUSE_SECRETSTORE
	  wxSecretValue(), // Creates an empty secret value (not the same as an empty password).
#endif
	  encodings, compresslevel, quality);
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
      
      list_box_services->Set(items);
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
      wxString msg = wxString::Format(_("Window sharing with %s stopped. Either the other side does not support receiving windows or the window was closed there."), cb->conn->getDesktopName());
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
      frame_main_menubar->Enable(wxID_UP, true);
      // "stop window share"
      frame_main_menubar->Enable(wxID_CANCEL, false);
    }
}

void MyFrameMain::onFullScreenChanged(wxFullScreenEvent &event) {
    wxLogDebug("onFullScreenChanged %d", event.IsFullScreen());
    // update this here as well as it might have been triggered from the WM buttons outside of our control
    show_fullscreen = event.IsFullScreen();
    wxString prefix = wxSystemSettings::GetAppearance().IsDark() ? "dark" : "light";
    if (show_fullscreen) {
	// tick menu item
	frame_main_menubar->Check(ID_FULLSCREEN, true);
        GetToolBar()->SetToolNormalBitmap(ID_FULLSCREEN, bitmapBundleFromSVGResource(prefix + "/" + "restore"));
#ifdef __WXMAC__
        // only disable affected view items
        frame_main_menubar->Enable(ID_BOOKMARKS, false);
        frame_main_menubar->Enable(ID_DISCOVERED, false);
#else
        // hide whole menu
	frame_main_menubar->Show(false);
#endif
	// hide bookmarks and discovered servers
	show_bookmarks = show_discovered = false;
	splitwinlayout();
        // hide toolbar labels
        GetToolBar()->SetWindowStyle(GetToolBar()->GetWindowStyle() & ~wxTB_TEXT);
        // hide and unlink status bar
        GetStatusBar()->Hide();
        SetStatusBar(nullptr);
    } else {
	// untick menu item
	frame_main_menubar->Check(ID_FULLSCREEN, false);
        GetToolBar()->SetToolNormalBitmap(ID_FULLSCREEN, bitmapBundleFromSVGResource(prefix + "/" + "fullscreen"));
#ifdef __WXMAC__
        // only enable affected view items
        frame_main_menubar->Enable(ID_BOOKMARKS, true);
        frame_main_menubar->Enable(ID_DISCOVERED, true);
#else
        // show whole menu again
	frame_main_menubar->Show(true);
#endif
	// restore bookmarks and discovered servers to saved state
	wxConfigBase::Get()->Read(K_SHOWDISCOVERED, &show_discovered, V_SHOWDISCOVERED);
	wxConfigBase::Get()->Read(K_SHOWBOOKMARKS, &show_bookmarks, V_SHOWBOOKMARKS);
	splitwinlayout();
        // show toolbar labels
        GetToolBar()->SetWindowStyle(GetToolBar()->GetWindowStyle() | wxTB_TEXT);
        // reattach and status bar
        SetStatusBar(frame_main_statusbar);
        GetStatusBar()->Show();
  }
    // needed at least on MacOS to let the status bar re-appear correctly on restore
    SendSizeEvent();
}


void MyFrameMain::onSysColourChanged(wxSysColourChangedEvent& event)
{
    wxString prefix = wxSystemSettings::GetAppearance().IsDark() ? "dark" : "light";
    GetToolBar()->SetToolNormalBitmap(wxID_YES, bitmapBundleFromSVGResource(prefix + "/" + "connect"));
    GetToolBar()->SetToolNormalBitmap(wxID_REDO, bitmapBundleFromSVGResource(prefix + "/"  + "listen"));
    GetToolBar()->SetToolNormalBitmap(wxID_STOP, bitmapBundleFromSVGResource(prefix + "/" + "disconnect"));
    GetToolBar()->SetToolNormalBitmap(ID_GRABKEYBOARD, bitmapBundleFromSVGResource(prefix + "/" + "toggle-keyboard-grab"));
    GetToolBar()->SetToolNormalBitmap(wxID_SAVE, bitmapBundleFromSVGResource(prefix + "/" + "screenshot"));
    GetToolBar()->SetToolNormalBitmap(ID_INPUT_RECORD, bitmapBundleFromSVGResource(prefix + "/" + "record"));
    GetToolBar()->SetToolNormalBitmap(ID_INPUT_REPLAY, bitmapBundleFromSVGResource(prefix + "/" + "replay"));
    GetToolBar()->SetToolNormalBitmap(ID_FULLSCREEN, bitmapBundleFromSVGResource(prefix + "/" + (show_fullscreen ? "restore" : "fullscreen")));
    GetToolBar()->SetToolNormalBitmap(ID_ONE_TO_ONE, bitmapBundleFromSVGResource(prefix + "/" + "one-to-one"));
}



void MyFrameMain::onVNCConnGetPasswordNotify(wxCommandEvent &event)
{
    // get sender
    VNCConn *conn = static_cast<VNCConn*>(event.GetEventObject());

    // Get password. We are only called if the password is needed!
    wxString pass = wxGetPasswordFromUser(_("Enter password:"), _("Password required!"));
    // And set password at conn.
#if wxUSE_SECRETSTORE
    conn->setPassword(pass.IsEmpty() ? wxSecretValue() : wxSecretValue(pass));
#else
    conn->setPassword(pass);
#endif
}


void MyFrameMain::onVNCConnGetCredentialsNotify(wxCommandEvent &event)
{
    // get sender
    VNCConn *conn = static_cast<VNCConn*>(event.GetEventObject());

    // stop showing connection setup busy cursor when entering creds
    wxEndBusyCursor();

    if(!event.GetInt()) {
	// without user prompt, get only password
	wxString pass = wxGetPasswordFromUser(wxString::Format(_("Please enter password for user '%s'"), conn->getUserName()),
					    _("Credentials required..."));
	// And set password at conn.
#if wxUSE_SECRETSTORE
	conn->setPassword(wxSecretValue(pass));
#else
	conn->setPassword(pass);
#endif
    } else {
	// with user prompt
        DialogLogin formLogin(0, wxID_ANY, _("Credentials required..."));
        if (formLogin.ShowModal() == wxID_OK) {
            conn->setUserName(formLogin.getUserName());
#if wxUSE_SECRETSTORE
            conn->setPassword(wxSecretValue(formLogin.getPassword()));
#else
            conn->setPassword(formLogin.getPassword());
#endif
        } else {
	    // canceled
#if wxUSE_SECRETSTORE
            conn->setPassword(wxSecretValue());
#else
            conn->setPassword(wxEmptyString);
#endif
        }
    }
}



bool MyFrameMain::saveStats(VNCConn* c, int conn_index, const wxArrayString& stats, wxString desc, bool autosave)
{
  if(stats.IsEmpty())
    {
      if(!autosave)
	wxLogMessage(_("Nothing to save!"));
      return true;
    }
  
  wxString filename = wxGetHostName() + wxT(" to ") + c->getDesktopName() + wxString::Format(wxT("(%i)"), conn_index) +
    wxT(" ") + desc + wxT(" stats on ") + wxNow() + wxT(".csv");
#ifdef __WIN32__
  // windows doesn't like ':'s
  filename.Replace(wxString(wxT(":")), wxString(wxT("-")));
#endif

  if(!autosave)
    filename = wxFileSelector(wxString::Format(_("Saving %s statistics..."), desc),
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

      for(size_t i=0; i < stats.GetCount(); ++i)
	ostream << stats[i].char_str() << endl;
    }

  return true;
}





// connection initiation and shutdown
void MyFrameMain::spawn_conn(wxString service, int listenPort)
{
  // get connection settings
  int compresslevel, quality, multicast_socketrecvbuf, multicast_recvbuf;
  bool multicast, enc_enabled;
  wxString encodings;
  wxConfigBase *pConfig = wxConfigBase::Get();
  pConfig->Read(K_MULTICAST, &multicast, V_MULTICAST);
  pConfig->Read(K_MULTICASTSOCKETRECVBUF, &multicast_socketrecvbuf, V_MULTICASTSOCKETRECVBUF);
  pConfig->Read(K_MULTICASTRECVBUF, &multicast_recvbuf, V_MULTICASTRECVBUF);

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

  if(listenPort > 0)
    {
      wxLogStatus(_("Listening on port") + " " + (wxString() << listenPort) + wxT(" ..."));
      c->Listen(listenPort);
    }
  else // normal init without previous listen
    {
      wxBeginBusyCursor();
      wxLogStatus(_("Connecting to %s..."), service);

      wxString user = service.Contains("@") ? service.BeforeFirst('@') : "";
      wxString host = service.Contains("@") ? service.AfterFirst('@') : service;
#if wxUSE_SECRETSTORE
      wxSecretValue password;
      wxSecretStore store = wxSecretStore::GetDefault();
      if (store.IsOk()) {
        wxString username; // this will not be used
        store.Load("MultiVNC/Bookmarks/" + service, username,
                   password); // if Load() fails, password will still be empty
      }
#endif
      c->Init(host, user,
#if wxUSE_SECRETSTORE
              password,
#endif
              encodings, compresslevel, quality, multicast,
              multicast_socketrecvbuf, multicast_recvbuf);
    }
}


void MyFrameMain::setup_conn(VNCConn *c) {

  // first, find out if we already setup this this connection.
  // this happens if it was a listening one that's now connected.
  vector<ConnBlob>::iterator it = connections.begin();
  size_t index = 0;
  while(it != connections.end() && it->conn != c)
      {
	  ++it;
	  ++index;
      }
  if (index < connections.size()) {
      // found it, just update the label and skip the rest we already did
      notebook_connections->SetPageText(index, c->getDesktopName() + " " + _("(Reverse Connection)"));
      return;
  }

  // get more settings
  int fastrequest_interval;
  bool fastrequest, qos_ef;
  wxConfigBase *pConfig = wxConfigBase::Get();
  pConfig->Read(K_FASTREQUEST, &fastrequest, V_FASTREQUEST);
  pConfig->Read(K_FASTREQUESTINTERVAL, &fastrequest_interval, V_FASTREQUESTINTERVAL);
  pConfig->Read(K_QOS_EF, &qos_ef, V_QOS_EF);

  if(show_stats)
    c->doStats(true);

  ViewerWindow* win = new ViewerWindow(notebook_connections, c);
  win->showStats(show_stats);
  win->showOneToOne(show_1to1);
#ifdef MULTIVNC_GRABKEYBOARD
  win->grabKeyboard(frame_main_toolbar->GetToolState(ID_GRABKEYBOARD));
#endif

  VNCSeamlessConnector* sc = 0;
  if(VNCSeamlessConnector::isSupportedByCurrentPlatform() && show_seamless != EDGE_NONE)
    sc = new VNCSeamlessConnector(this, c, show_seamless);

  ConnBlob cb;
  cb.conn = c;
  cb.viewerwindow = win;
  cb.seamlessconnector = sc;  
  cb.windowshare_proc = 0;
  cb.windowshare_proc_pid = 0;

  connections.push_back(cb);

  if(!c->getListenPort().IsEmpty())
    notebook_connections->AddPage(win, _("Listening on port") + " " + c->getListenPort(), true);
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
  frame_main_menubar->Enable(wxID_STOP, true);
  // "screenshot"
  frame_main_menubar->Enable(wxID_SAVE, true);
  // stats
  frame_main_menubar->Enable(ID_STATS_SAVE, true);
  // record/replay
  frame_main_menubar->Enable(ID_INPUT_RECORD, true);
  frame_main_menubar->Enable(ID_INPUT_REPLAY, true);
  // bookmarks
  frame_main_menubar->Enable(wxID_ADD, true);
  // window sharing
  frame_main_menubar->Enable(wxID_UP, true);
  frame_main_menubar->Enable(wxID_CANCEL, false);

  if(GetToolBar())
    {
      GetToolBar()->EnableTool(wxID_STOP, true); // disconnect
      GetToolBar()->EnableTool(wxID_SAVE, true); // screenshot
      GetToolBar()->EnableTool(ID_INPUT_REPLAY, true);
      GetToolBar()->EnableTool(ID_INPUT_RECORD, true);
    }

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
	  if(!saveStats(c, index, c->getStats(), c->isMulticast() ? wxT("MulticastVNC") : wxT("VNC"), true))
	    wxLogError(_("Could not autosave statistics!"));   
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
      frame_main_menubar->Enable(wxID_STOP, false);
      // "screenshot"
      frame_main_menubar->Enable(wxID_SAVE, false);
      // stats
      frame_main_menubar->Enable(ID_STATS_SAVE, false);
      // record/replay
      frame_main_menubar->Enable(ID_INPUT_RECORD, false);
      frame_main_menubar->Enable(ID_INPUT_REPLAY, false);
      // bookmarks
      frame_main_menubar->Enable(wxID_ADD, false);
      // window sharing
      frame_main_menubar->Enable(wxID_UP, false);
      frame_main_menubar->Enable(wxID_CANCEL, false);

      if(GetToolBar())
	{
	  GetToolBar()->EnableTool(wxID_STOP, false); // disconnect
	  GetToolBar()->EnableTool(wxID_SAVE, false); // screenshot
	  GetToolBar()->EnableTool(ID_INPUT_REPLAY, false);
	  GetToolBar()->EnableTool(ID_INPUT_RECORD, false);
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
	frame_main_menubar->Enable(wxID_EDIT, false);
	frame_main_menubar->Enable(wxID_DELETE, false);
    }  
  else
    {
      if(list_box_bookmarks->GetSelection() >= 0)
	{
	    frame_main_menubar->Enable(wxID_EDIT, true);
	    frame_main_menubar->Enable(wxID_DELETE, true);
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
  wxMenu* bm_menu = frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(_("Bookmarks")));
  for(int i = bm_menu->GetMenuItemCount()-1; i > 3; --i)
    bm_menu->Destroy(bm_menu->FindItemByPosition(i));
  bm_menu->AppendSeparator();


  // then read in each bookmark value pair
  for(size_t i=0; i < bookmarknames.GetCount(); ++i)
    {
      wxString host, port, user;

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

      // user is optional
      cfg->Read(K_BOOKMARKS_USER, &user);

      // add brackets if host is an IPv6 address
      if(host.Freq(':') > 0)
	 host = wxT("[") + host + wxT("]");

      // all fine, add it
      bookmarks.Add((user != wxEmptyString ? user + "@" : "") + host + wxT(":") + port);

      // and add to bookmarks menu
      int id = NewControlId();
      wxString* index_str = new wxString; // pack i into a wxObject, we use wxString here
      *index_str << i;
      bm_menu->Append(id, bookmarknames[i]);
      bm_menu->SetHelpString(id, _("Bookmark") + " " + host + wxT(":") + port);
      Connect(id, wxEVT_COMMAND_MENU_SELECTED, 
	      wxCommandEventHandler(FrameMain::listbox_bookmarks_dclick), (wxObject*)index_str);
     }
  
  cfg->SetPath(wxT("/"));

  list_box_bookmarks->Set(bookmarknames);

  return true;
}





/*
  public members
*/


void MyFrameMain::machine_connect(wxCommandEvent &event)
{
    wxConfigBase *pConfig = wxConfigBase::Get();
    wxString host;
    pConfig->Read(K_LASTHOST, &host);

  wxString s = wxGetTextFromUser(_("Enter host to connect to:"),
				 _("Connect to a specific host."),
				 host);

  if (s != wxEmptyString) {
      pConfig->Write(K_LASTHOST, s);
      spawn_conn(s);
  }
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
  spawn_conn(wxEmptyString, port);
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



void MyFrameMain::machine_grabkeyboard(wxCommandEvent &event)
{
  if(connections.size())
    connections.at(notebook_connections->GetSelection()).viewerwindow->grabKeyboard(event.IsChecked());
}



void MyFrameMain::machine_save_stats(wxCommandEvent &event)
{
  if(connections.size())
    {
      int sel = notebook_connections->GetSelection();
      VNCConn* c = connections.at(sel).conn;
      saveStats(c, sel, c->getStats(), c->isMulticast() ? wxT("MulticastVNC") : wxT("VNC"), false);
    }
}



void MyFrameMain::machine_input_record(wxCommandEvent &event)
{
  if(connections.size())
    {
      int sel = notebook_connections->GetSelection();
      VNCConn* c = connections.at(sel).conn;
      
      if(c->isReplaying())
	return; // bail out


      if(c->isRecording())
	{
          wxString prefix = wxSystemSettings::GetAppearance().IsDark() ? "dark" : "light";
	  frame_main_toolbar->SetToolNormalBitmap(ID_INPUT_RECORD, bitmapBundleFromSVGResource(prefix + "/" + "record"));
          frame_main_toolbar->FindById(ID_INPUT_RECORD)->SetLabel(_("Record Input"));
	  
	  wxArrayString recorded_input;
	  
	  if(c->recordUserInputStop(recorded_input))
	    {
	      wxLogStatus(_("Stopped recording user input!"));
	      frame_main_menubar->SetLabel(ID_INPUT_RECORD,_("Record Input"));

	      // re-enable replay buttons
	      GetToolBar()->EnableTool(ID_INPUT_REPLAY, true);
	      frame_main_menubar->Enable(ID_INPUT_REPLAY, true);
	      
	      if(recorded_input.IsEmpty())
		{
		  wxLogMessage(_("Nothing to save!"));
		  return;
		}

	      wxString filename = wxGetHostName() + wxT(" to ") + c->getDesktopName() + wxString::Format(wxT("(%i)"), sel) +
	        + wxT(" input on ") + wxNow() + wxT(".csv");
#ifdef __WIN32__
	      // windows doesn't like ':'s
	      filename.Replace(wxString(wxT(":")), wxString(wxT("-")));
#endif
	      
	      filename = wxFileSelector(_("Save recorded input..."), 
					wxEmptyString,
					filename, 
					wxT(".csv"), 
					_("CSV files|*.csv"), 
					wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
 
	      wxLogDebug(wxT("About to save recorded input to ") + filename);

	      if(!filename.empty())
		{
		  wxBusyCursor busy;
		  ofstream ostream(filename.char_str());
		  if(! ostream)
		    {
		      wxLogError(_("Could not save file!"));
		      return;
		    }

		  for(size_t i=0; i < recorded_input.GetCount(); ++i)
		    ostream << recorded_input[i].char_str() << endl;
		}
	    }

	}
      else // not recording
	{
	  wxLogMessage(_("From now on, all your mouse and keyboard input will be recorded. Click the stop button to finish recording and save your input."));

	  if( c->recordUserInputStart()) 
	    {
              wxString prefix = wxSystemSettings::GetAppearance().IsDark() ? "dark" : "light";
	      frame_main_toolbar->SetToolNormalBitmap(ID_INPUT_RECORD, bitmapBundleFromSVGResource(prefix + "/" + "stop"));
              frame_main_toolbar->FindById(ID_INPUT_RECORD)->SetLabel(_("Stop"));
	      frame_main_menubar->SetLabel(ID_INPUT_RECORD, _("Stop Recording"));

	      wxLogStatus(_("Recording user input..."));

	      // disable replay buttons
	      GetToolBar()->EnableTool(ID_INPUT_REPLAY, false);
	      frame_main_menubar->Enable(ID_INPUT_REPLAY, false);
	    }
	}
	
    }
}


void MyFrameMain::machine_input_replay(wxCommandEvent &event)
{
  bool shift_was_down = wxGetMouseState().ShiftDown();

  if(connections.size())
    {
      int sel = notebook_connections->GetSelection();
      VNCConn* c = connections.at(sel).conn;
      
      if(c->isRecording())
	return; // bail out


      if(c->isReplaying())
	{
	  c->replayUserInputStop();

          wxString prefix = wxSystemSettings::GetAppearance().IsDark() ? "dark" : "light";
	  frame_main_toolbar->SetToolNormalBitmap(ID_INPUT_REPLAY, bitmapBundleFromSVGResource(prefix + "/" + "replay"));
          frame_main_toolbar->FindById(ID_INPUT_REPLAY)->SetLabel(_("Replay Input"));
	  frame_main_menubar->SetLabel(ID_INPUT_REPLAY,_("Replay Input"));

	  wxLogStatus(_("Stopped replaying user input!"));

	  // re-enable record buttons
	  GetToolBar()->EnableTool(ID_INPUT_RECORD, true);
	  frame_main_menubar->Enable(ID_INPUT_RECORD, true);
	}
      else // not replaying
	{
	  wxArrayString recorded_input;

	  // load recorded input
	  wxString  filename = wxFileSelector(_("Load recorded input..."), 
					      wxEmptyString,
					      wxEmptyString, 
					      wxT(".csv"), 
					      _("CSV files|*.csv"), 
					      wxFD_OPEN|wxFD_FILE_MUST_EXIST);
 
	  if(!filename.empty())
	    {
	      wxBusyCursor busy;
	      
	      wxLogDebug(wxT("About to load recorded input from ") + filename);
	      
	      ifstream istream(filename.char_str());
	      if(! istream)
		{
		  wxLogError(_("Could not open file!"));
		  return;
		}

	      char buf[1024];
	      while(istream.getline(buf, 1024))
		recorded_input.Add(wxString(buf, wxConvUTF8));
	    }
	  else
	    return; // canceled

	  
	  // start replay
	  if(c->replayUserInputStart(recorded_input, shift_was_down))
	    {
              wxString prefix = wxSystemSettings::GetAppearance().IsDark() ? "dark" : "light";
	      frame_main_toolbar->SetToolNormalBitmap(ID_INPUT_REPLAY, bitmapBundleFromSVGResource(prefix + "/" + "stop"));
	      frame_main_menubar->SetLabel(ID_INPUT_REPLAY, _("Stop Replaying"));
              frame_main_toolbar->FindById(ID_INPUT_REPLAY)->SetLabel(_("Stop"));

	      if(shift_was_down)
		wxLogStatus(_("Replaying user input in loop..."));
	      else
		wxLogStatus(_("Replaying user input..."));

	      // disable record buttons
	      GetToolBar()->EnableTool(ID_INPUT_RECORD, false);
	      frame_main_menubar->Enable(ID_INPUT_RECORD, false);
	    }
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

  frame_main_toolbar->Show(show_toolbar);

  if(show_toolbar)
    {
      bool enable = connections.size() != 0;
	
      frame_main_toolbar->EnableTool(wxID_STOP, enable); // disconnect
      frame_main_toolbar->EnableTool(wxID_SAVE, enable); // screenshot
      
      frame_main_toolbar->Show();
    }
  else
    {
        frame_main_toolbar->Hide();
    }

  // this does more than Layout() which only deals with sizers
  SendSizeEvent(); 

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

  wxLogDebug("view_togglefullscreen %d", show_fullscreen);

#ifdef __WXMAC__
  ShowFullScreen(show_fullscreen);
#else
  ShowFullScreen(show_fullscreen, wxFULLSCREEN_NOBORDER | wxFULLSCREEN_NOCAPTION);
#endif

  // according to https://docs.wxwidgets.org/3.2/classwx_full_screen_event.html
  // the event is not fired when using ShowFullScreen(), so manually do this here
  // (it is fired on OSX when entering fullscreen via the green button, so we need to
  // have the extra event handler).
  wxFullScreenEvent e = wxFullScreenEvent(0, show_fullscreen);
  onFullScreenChanged(e);
}


void MyFrameMain::view_toggle1to1(wxCommandEvent &event)
{
    show_1to1 = ! show_1to1;
    wxLogDebug("view_toggle1to1 %d", show_1to1);

    // keep toolbar and menu entries in sync
    frame_main_menubar->Check(ID_ONE_TO_ONE, show_1to1);
    GetToolBar()->ToggleTool(ID_ONE_TO_ONE, show_1to1);

    // for now, toggle all connections
    for(size_t i=0; i < connections.size(); ++i) {
        connections.at(i).viewerwindow->showOneToOne(show_1to1);
    }

    wxConfigBase *pConfig = wxConfigBase::Get();
    pConfig->Write(K_SHOW1TO1, show_1to1);
}




void MyFrameMain::view_seamless(wxCommandEvent &event)
{
  if(frame_main_menubar->IsChecked(ID_SEAMLESS_NORTH))
    show_seamless = EDGE_NORTH;
  else if
    (frame_main_menubar->IsChecked(ID_SEAMLESS_EAST))
    show_seamless = EDGE_EAST;
  else if 
    (frame_main_menubar->IsChecked(ID_SEAMLESS_WEST))
    show_seamless = EDGE_WEST;
  else if
    (frame_main_menubar->IsChecked(ID_SEAMLESS_SOUTH))
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
				    _("Saving bookmark"),
				    (! c->getUserName().IsEmpty() ? c->getUserName() + "@" : "") + c->getDesktopName());
				
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
      cfg->Write(K_BOOKMARKS_USER, c->getUserName());

#if wxUSE_SECRETSTORE
      wxSecretStore store = wxSecretStore::GetDefault();
      if(store.IsOk() && c->getPassword().IsOk()) { //check if destination and source are ok
	  if(!store.Save("MultiVNC/Bookmarks/" + (c->getUserName().IsEmpty() ? "" : c->getUserName() + "@") + c->getServerHost() + ":" + c->getServerPort(), c->getUserName(), c->getPassword())) //FIXME the service should use the user-given bookmark name, but that requires a rework of our internal bookmarking
	      wxLogWarning(_("Failed to save credentials to the system secret store."));
      }
#endif

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
  wxString name = list_box_bookmarks->GetStringSelection();
  
  if(name.IsEmpty())
    {
      wxLogError(_("No bookmark selected!"));
      return;
    }

  wxConfigBase *cfg = wxConfigBase::Get();
  if(!cfg->DeleteGroup(G_BOOKMARKS + name))
    wxLogError(_("No bookmark with this name!"));

#if wxUSE_SECRETSTORE
  int sel = list_box_bookmarks->GetSelection();
  if(sel != wxNOT_FOUND) {
      wxString service = bookmarks[sel];
      wxSecretStore store = wxSecretStore::GetDefault();
      if(store.IsOk())
	  store.Delete("MultiVNC/Bookmarks/" + service);
  }
#endif

  // and re-read
  loadbookmarks();
}





void MyFrameMain::help_about(wxCommandEvent &event)
{	
  wxAboutDialogInfo info;
  wxIcon icon = bitmapBundleFromSVGResource("about").GetIcon(this->FromDIP(wxSize(128,128)));

  wxString desc = "\n";
  desc += _("MultiVNC is a cross-platform Multicast-enabled VNC client.");
  desc += "\n\n";
  desc += _("Built with") + " " + (wxString() << wxVERSION_STRING);
  desc += "\n\n";
  desc += _("Supported Security Types:");
  desc += "\n";
  desc += _("VNC Authentication");
#if defined LIBVNCSERVER_HAVE_GNUTLS || defined LIBVNCSERVER_HAVE_LIBSSL
  desc += wxT(", Anonymous TLS, VeNCrypt");
#endif
#if defined LIBVNCSERVER_HAVE_LIBGCRYPT || defined LIBVNCSERVER_HAVE_LIBSSL
  desc += wxT(", Apple Remote Desktop");
#endif
  desc += "\n\n";
  desc += _("Supported Encodings:");
  desc += "\n";
  desc += wxT("Raw, RRE, coRRE, CopyRect, Hextile, Ultra");
#ifdef LIBVNCSERVER_HAVE_LIBZ 
  desc += wxT(", UltraZip, Zlib, ZlibHex, ZRLE, ZYWRLE");
#ifdef LIBVNCSERVER_HAVE_LIBJPEG 
  desc += wxT(", Tight");
#endif
#endif

#ifndef __WXMAC__
  info.SetIcon(icon);
  info.SetVersion(VERSION);
  info.SetWebSite(wxString(PACKAGE_URL));
#endif
  info.SetName(wxT("MultiVNC"));
  info.SetDescription(desc);
  info.SetCopyright(wxT(COPYRIGHT));
  info.AddDeveloper("Christian Beier");
  info.AddDeveloper("Evgeny Zinoviev");
  info.AddDeveloper("Audrey Dutcher");
  
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



void MyFrameMain::help_issue_list(wxCommandEvent &e)
{
    wxString platform;
#if defined __LINUX__
    platform = "linux";
#elif defined __WINDOWS__
    platform = "windows";
#elif defined __APPLE__
    platform = "mac";
#endif
    wxLaunchDefaultBrowser("https://github.com/bk138/multivnc/issues?q=is%3Aissue+is%3Aopen+label%3Aplatform-" + platform);
}



void MyFrameMain::listbox_services_select(wxCommandEvent &event)
{
    // intentionally empty
    wxLogStatus(_("VNC Server") + " " + list_box_services->GetStringSelection());
}


void MyFrameMain::listbox_services_dclick(wxCommandEvent &event)
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
  
    timeout = 5000;
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

  // check if we actually have to resolve the IP address ourselves
  bool is_system_resolving_mdns = false;
#ifdef __WXMAC__
  is_system_resolving_mdns = true;
#else
  wxLog::EnableLogging(false);
  wxFileInputStream input("/etc/nsswitch.conf");
  wxLog::EnableLogging(true);
  wxTextInputStream text(input);
  while(input.IsOk() && !input.Eof())
      if(text.ReadLine().Contains("mdns")) {
	  is_system_resolving_mdns = true;
	  wxLogDebug("System resolver does mDNS, skipping IP address lookup");
	  break;
      }
#endif

  if(!is_system_resolving_mdns)
  // lookup ip address
  {
    wxServDisc addrscan(0, services_hostname, QTYPE_A);
  
    timeout = 5000;
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
  else
      services_addr = services_hostname; // system resolves mDNS

  wxLogStatus(services_hostname + wxT(" (") + services_addr + wxT(":") + services_port + wxT(")"));


  spawn_conn(services_addr + wxT(":") + services_port);
} 
 


void MyFrameMain::listbox_bookmarks_select(wxCommandEvent &event)
{
  int sel = event.GetInt();

  if(sel < 0) //nothing selected
    {
      frame_main_menubar->Enable(wxID_EDIT, false);
      frame_main_menubar->Enable(wxID_DELETE, false);
      
      return;
    }
  else
    {
      frame_main_menubar->Enable(wxID_EDIT, true);
      frame_main_menubar->Enable(wxID_DELETE, true);
     
      wxLogStatus(_("Bookmark %s"), bookmarks[sel]);
    }
}


void MyFrameMain::listbox_bookmarks_dclick(wxCommandEvent &event)
{
  int sel = event.GetInt();

  if(sel < 0) { // nothing selected
      // this gets set by Connect() in loadbookmarks(), in this case sel is always -1
      if(event.m_callbackUserData)
	  sel = wxAtoi(*(wxString*)event.m_callbackUserData);
      else
	  return;
  }

  spawn_conn(bookmarks[sel]);
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
  frame_main_menubar->Enable(wxID_UP, !isSharing);
  // this is "stop share window"
  frame_main_menubar->Enable(wxID_CANCEL, isSharing);

  if(cb->seamlessconnector) 
    {
      switch(cb->seamlessconnector->getEdge())
	{
	case EDGE_NORTH:
	  frame_main_menubar->Check(ID_SEAMLESS_NORTH, true);
	  break;
	case EDGE_EAST:
	  frame_main_menubar->Check(ID_SEAMLESS_EAST, true);
	  break;
	case EDGE_WEST:
	  frame_main_menubar->Check(ID_SEAMLESS_WEST, true);
	  break;
	case EDGE_SOUTH:
	  frame_main_menubar->Check(ID_SEAMLESS_SOUTH, true);
	  break;
	default:
	  frame_main_menubar->Check(ID_SEAMLESS_DISABLED, true);
	}

      cb->seamlessconnector->Raise();
    }
  else // no object, i.e. disabled
    frame_main_menubar->Check(ID_SEAMLESS_DISABLED, true);
}



void MyFrameMain::cmdline_connect(wxString& hostarg)
{
  spawn_conn(hostarg);
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
  if(wxMessageBox(_("The MultiVNC window will be minimized and a cross-shaped cursor will appear. Use it to select the window you want to share."),
		  _("Share a Window"), wxOK|wxCANCEL)
     == wxCANCEL)
    return;
  
  Iconize(true);
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

  SetStatusText(wxString::Format(_("Sharing window with %s"), cb->conn->getDesktopName()));
		
  // this is "share window"
  frame_main_menubar->Enable(wxID_UP, false);
  // this is "stop share window"
  frame_main_menubar->Enable(wxID_CANCEL, true);
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
      
      wxLogStatus(_("Stopped sharing window with %s"), cb->conn->getDesktopName());

      // this is "share window"
      frame_main_menubar->Enable(wxID_UP, true);
      // this is "stop share window"
      frame_main_menubar->Enable(wxID_CANCEL, false);
    }
  else
    wxLogDebug(wxT("windowshare_stop(): Could not kill %d. Not good."), cb->windowshare_proc_pid);
}




