
#include "gui/bitmapFromMem.h"
#include "gui/evtids.h"
#include "uri.h"
#include <fstream>
#include <wx/aboutdlg.h>
#include <wx/textctrl.h>
#include <wx/creddlg.h>
#include <wx/socket.h>
#include <wx/clipbrd.h>
#include <wx/imaglist.h>
#include <wx/richmsgdlg.h>
#include <wx/secretstore.h>
#include <wx/tokenzr.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>

#include "MyFrameMain.h"
#include "MyDialogSettings.h"
#include "MyDialogNewConnection.h"
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
  EVT_COMMAND (wxID_ANY, VNCConnSshFingerprintMismatchNOTIFY, MyFrameMain::onVNCConnSshFingerprintMismatchNotify)
  EVT_VNCCONNX509FINGERPRINTMISMATCHNOTIFY (wxID_ANY, MyFrameMain::onVNCConnX509FingerprintMismatchNotify)
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


  // assign images to notebook_connections
  wxVector<wxBitmapBundle> images;
  images.push_back(bitmapBundleFromSVGResource("unicast"));
  images.push_back(bitmapBundleFromSVGResource("multicast"));
  notebook_connections->SetImages(images);

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

  bookmarks_load_to_list();

  if(show_discovered)
    frame_main_menubar->Check(ID_DISCOVERED, true);
  if(show_bookmarks)
    frame_main_menubar->Check(ID_BOOKMARKS, true);
  if(show_stats)
    frame_main_menubar->Check(ID_STATISTICS, true);

  if (VNCSeamlessConnector::isSupportedByCurrentPlatform()) {
      switch (show_seamless) {
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

  // right click handler for bookmarks
  list_box_bookmarks->Bind(wxEVT_CONTEXT_MENU, &MyFrameMain::listbox_bookmarks_context, this);
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
    conn_terminate(i);

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
        // successfully entered listening state, just update the tab label
        ConnBlob *cb;
        vector<ConnBlob>::iterator it = connections.begin();
        size_t index = 0;
        while(it != connections.end() && it->conn != c) {
            ++it;
            ++index;
        }
        if (index < connections.size()) {
            notebook_connections->SetPageText(index, _("Listening on port") + " " + c->getListenPort());
        }
    } else {
	// listen error
        wxLogStatus(_("Listening failed."));
        // We want a modal dialog here so that the viewer window closes after the dialog.
        // The title is translated in wx itself.
        wxRichMessageDialog dialog (this,
                                    c->getErr(),
                                    wxString::Format(_("%s Error"), wxTheApp->GetAppDisplayName()),
                                    wxICON_ERROR|wxHELP);
        // show last 3 log strings
        wxArrayString log = VNCConn::getLog();
        wxString detailed;
        for (size_t i = log.GetCount() >= 3 ? log.GetCount() - 3 : 0; i < log.GetCount(); ++i) {
            detailed += log[i];
        }
        dialog.ShowDetailedText(detailed);
        dialog.SetHelpLabel(_("Show &Log"));
        if(dialog.ShowModal() == wxID_HELP) {
            wxCommandEvent unused;
            machine_showlog(unused);
        }

        // clean up
	vector<ConnBlob>::iterator it = connections.begin();
	size_t index = 0;
	while(it != connections.end() && it->conn != c)  {
		++it;
		++index;
        }
	if (index < connections.size()) {
	    conn_terminate(index);
        }
    }
}

void MyFrameMain::onVNCConnInitNotify(wxCommandEvent& event)
{
    VNCConn* c = static_cast<VNCConn*>(event.GetEventObject());

    // first, find index of this VNCConn's ConnBlob
    vector<ConnBlob>::iterator it = connections.begin();
    size_t index = 0;
    while(it != connections.end() && it->conn != c) {
        ++it;
        ++index;
    }

    switch(event.GetInt()) {
    case VNCConn::InitState::CONNECT_SUCCESS:
        // update page label
        {
            wxString label;
            if (c->getSshHost().IsEmpty()) {
                if (c->getRepeaterId().IsEmpty()) {
                    label = wxString::Format(_("Connected to %s"),c->getServerHost());
                } else {
                    label = wxString::Format(_("Connected to %s, waiting for peer %s"), c->getServerHost(), c->getRepeaterId());
                }
            } else {
                if (c->getRepeaterId().IsEmpty()) {
                    label = wxString::Format(_("Connected to %s via %s"),c->getServerHost(), c->getSshHost());
                } else {
                    label = wxString::Format(_("Connected to %s via %s, waiting for peer %s"), c->getSshHost(), c->getServerHost(), c->getRepeaterId());
                }
            }
            notebook_connections->SetPageText(index, label);
        }
        break;
    case VNCConn::InitState::CONNECT_FAIL:
    case VNCConn::InitState::INITIALISE_FAIL:
        // error. only show error if this was not a auth case with empty password, i.e. a canceled ones
        if (c->getRequireAuth()
            && !c->getPassword().IsOk()) {
            wxLogStatus(_("Authentication canceled."));
        } else {
            wxLogStatus(_("Connection failed."));
            // We want a modal dialog here so that the viewer window closes after the dialog.
            // The title is translated in wx itself.
            wxRichMessageDialog dialog (this,
                                        c->getErr(),
                                        wxString::Format(_("%s Error"), wxTheApp->GetAppDisplayName()),
                                        wxICON_ERROR|wxHELP);
            // show last 3 log strings
            wxArrayString log = VNCConn::getLog();
            wxString detailed;
            for (size_t i = log.GetCount() >= 3 ? log.GetCount() - 3 : 0; i < log.GetCount(); ++i) {
                detailed += log[i];
            }
            dialog.ShowDetailedText(detailed);
            dialog.SetHelpLabel(_("Show &Log"));
            if(dialog.ShowModal() == wxID_HELP) {
                wxCommandEvent unused;
                machine_showlog(unused);
            }
        }
        // fall through to cancel cases
    case VNCConn::InitState::CONNECT_CANCEL:
    case VNCConn::InitState::INITIALISE_CANCEL:
        if (index < connections.size()) {
            conn_terminate(index);
        }
        break;
    case VNCConn::InitState::INITIALISE_SUCCESS:
        conn_setup(c);
        break;
    };
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

  if (event.GetInt() == 1) {
      // Disconnect by remote side.
      // We want a modal dialog here so that the viewer window closes after the dialog.
      // The title is translated in wx itself.
      wxString message;
      if (!c->getServerHost().IsEmpty()) {
          message = wxString::Format(_("Connection to %s:%s terminated."), c->getServerHost().c_str(), c->getServerPort().c_str());
      } else {
          message = _("Reverse connection terminated.");
      }
      wxRichMessageDialog dialog (this,
                                  message,
                                  wxString::Format(_("%s Information"), wxTheApp->GetAppDisplayName()),
                                  wxICON_INFORMATION|wxHELP);
      // show last 3 log strings
      wxArrayString log = VNCConn::getLog();
      wxString detailed;
      for (size_t i = log.GetCount() >= 3 ? log.GetCount() - 3 : 0; i < log.GetCount(); ++i) {
          detailed += log[i];
      }
      dialog.ShowDetailedText(detailed);
      dialog.SetHelpLabel(_("Show &Log"));
      if(dialog.ShowModal() == wxID_HELP) {
          wxCommandEvent unused;
          machine_showlog(unused);
      }
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
    conn_terminate(index);
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

  if(!c->Init(wxEmptyString,
              5900,
              -1,
              wxEmptyString,
              wxSecretValue(), // Creates an empty secret value (not the same as an empty password).
              wxEmptyString, // incoming connections cannot be SSH-tunneled
              -1,
              wxEmptyString,
              wxEmptyString,
              wxSecretValue(),
              wxEmptyString,
              wxSecretValue(),
              wxEmptyString,
              encodings, compresslevel, quality)) {
      wxLogError(c->getErr());
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
    conn->setPassword(pass.IsEmpty() ? wxSecretValue() : wxSecretValue(pass));
}


void MyFrameMain::onVNCConnGetCredentialsNotify(wxCommandEvent &event)
{
    // get sender
    VNCConn *conn = static_cast<VNCConn*>(event.GetEventObject());

    if(!event.GetInt()) {
	// without user prompt, get only password
	wxString pass = wxGetPasswordFromUser(wxString::Format(_("Please enter password for user '%s'"), conn->getUserName()),
					    _("Credentials required..."));
	// And set password at conn.
	conn->setPassword(wxSecretValue(pass));
    } else {
	// with user prompt
        wxCredentialEntryDialog formLogin(this, wxEmptyString, _("Credentials required..."));
        if (formLogin.ShowModal() == wxID_OK) {
            conn->setUserName(formLogin.GetCredentials().GetUser());
            conn->setPassword(formLogin.GetCredentials().GetPassword());
        } else {
	    // canceled
            conn->setPassword(wxSecretValue());
        }
    }
}


void MyFrameMain::onVNCConnSshFingerprintMismatchNotify(wxCommandEvent &event) {
    // get sender
    VNCConn *conn = static_cast<VNCConn*>(event.GetEventObject());
    wxString remoteFingerprint = event.GetString(); // comes as lowercase hex

    if (conn->getSshFingerprintExpected().IsEmpty()) {
        // We don't want the optional "connect w/o saving fingerprint", SSH does not have it either
        wxMessageDialog dialog(this,
                               wxString::Format(_("This is the first time you are connecting to the SSH server '%s'.\n\n"
                                                  "Please verify the server's fingerprint to ensure you are "
                                                  "connecting to the correct server:\n\n"
                                                  "SHA256:%s\n\n"
                                                  "Do you want to trust this server and save its fingerprint for future connections?"),
                                                conn->getSshHost() + ":" + conn->getSshPort(),
                                                hex_to_base64(remoteFingerprint)),
                               _("Verify SSH Server Identity"),
                               wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
        if (dialog.ShowModal() == wxID_YES) {
            ssh_known_hosts_save(conn->getSshHost(), conn->getSshPort(), remoteFingerprint);
            conn->setSshFingerprintMismatchAccept(true);
        } else {
            conn->setSshFingerprintMismatchAccept(false);
        }
    } else {
        wxMessageDialog dialog(this,
                               wxString::Format(_("The fingerprint of the SSH server '%s' has changed since your last connection!"),
                                                conn->getSshHost() + ":" + conn->getSshPort()),
                               "Security Warning: SSH Fingerprint Mismatch",
                               wxYES_NO | wxHELP | wxNO_DEFAULT | wxICON_WARNING);
        dialog.SetExtendedMessage(wxString::Format(_( "This could indicate a security risk, such as a man-in-the-middle attack. "
                                                      "It is also possible that a host key has just been changed.\n"
                                                      " \n"
                                                      "Expected fingerprint: SHA256:%s\n"
                                                      "Received fingerprint: SHA256:%s\n"),
                                                   hex_to_base64(ssh_known_hosts_load(conn->getSshHost(), conn->getSshPort())),
                                                   hex_to_base64(remoteFingerprint)));
        dialog.SetHelpLabel(_("Connect and update fingerprint"));
        dialog.SetYesNoLabels(_("Connect without updating fingerprint"), _("Cancel"));
        int result = dialog.ShowModal();
        if (result == wxID_HELP) {
            ssh_known_hosts_save(conn->getSshHost(), conn->getSshPort(), remoteFingerprint);
            conn->setSshFingerprintMismatchAccept(true);
        }
        else if (result == wxID_YES) {
            conn->setSshFingerprintMismatchAccept(true);
        }
        else {
            conn->setSshFingerprintMismatchAccept(false);
        }
    }
}

void MyFrameMain::onVNCConnX509FingerprintMismatchNotify(VNCConnX509FingerprintMismatchNotifyEvent &event) {
    // get sender
    VNCConn *conn = static_cast<VNCConn*>(event.GetEventObject());

    char time_buf[128];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S UTC", gmtime(&event.remoteCertValidFrom));
    wxString remoteCertValidFrom = wxString::FromUTF8(time_buf);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S UTC", gmtime(&event.remoteCertValidUntil));
    wxString remoteCertValidUntil = wxString::FromUTF8(time_buf);

    if (conn->getX509FingerprintExpected().IsEmpty()) {
        // We don't want the optional "connect w/o saving fingerprint"
        wxMessageDialog dialog(this,
                               wxString::Format(_("The server's identity could not be verified automatically.\n"
                                                  "\n"
                                                  "This may indicate a security risk (e.g., a man-in-the-middle attack) or the server may be using a self-signed or untrusted certificate.\n"
                                                  "\n"
                                                  "Certificate Details:\n"
                                                  " - Subject: %s\n"
                                                  " - Valid From: %s\n"
                                                  " - Valid Until: %s\n"
                                                  " - SHA-256 Fingerprint: %s\n"
                                                  "\n"
                                                  "Do you want to trust this certificate and continue? Select 'Yes' only if you are sure the fingerprint matches the expected server."),
                                                event.remoteCertSubject,
                                                remoteCertValidFrom,
                                                remoteCertValidUntil,
                                                hex_to_colon_separated(event.remoteCertSha256Fingerprint)),
                               _("Unverified Server Certificate"),
                               wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
        if (dialog.ShowModal() == wxID_YES) {
            x509_known_hosts_save(conn->getServerHost(), conn->getServerPort(), event.remoteCertSha256Fingerprint);
            conn->setX509FingerprintMismatchAccept(true);
        } else {
            conn->setX509FingerprintMismatchAccept(false);
        }
    } else {
        wxMessageDialog dialog(this,
                               _( "WARNING: The server's certificate does NOT match the expected fingerprint."),
                               "Security Warning: Server Certificate Mismatch",
                               wxYES_NO | wxHELP | wxNO_DEFAULT | wxICON_WARNING);
        dialog.SetExtendedMessage(wxString::Format(_("This could indicate a security attack (e.g., a man-in-the-middle attack) or a misconfigured server.\n"
                                                     "\n"
                                                     "Expected Certificate:\n"
                                                     "- SHA-256 Fingerprint: %s\n"
                                                     "\n"
                                                     "Server's Certificate:\n"
                                                     "- Subject: %s\n"
                                                     "- Valid From: %s\n"
                                                     "- Valid Until: %s\n"
                                                     "- SHA-256 Fingerprint: %s\n"
                                                     "\n"
                                                     "Connecting to this server is UNSAFE unless you explicitly trust this new certificate.\n"
                                                     "\n"
                                                     "Do you want to proceed anyway? Only select 'Yes' if you have manually verified the new fingerprint."),
                                                   hex_to_colon_separated(conn->getX509FingerprintExpected()),
                                                   event.remoteCertSubject,
                                                   remoteCertValidFrom,
                                                   remoteCertValidUntil,
                                                   hex_to_colon_separated(event.remoteCertSha256Fingerprint)));
        dialog.SetHelpLabel(_("Connect and update fingerprint"));
        dialog.SetYesNoLabels(_("Connect without updating fingerprint"), _("Cancel"));
        int result = dialog.ShowModal();
        if (result == wxID_HELP) {
            x509_known_hosts_save(conn->getServerHost(), conn->getServerPort(), event.remoteCertSha256Fingerprint);
            conn->setX509FingerprintMismatchAccept(true);
        }
        else if (result == wxID_YES) {
            conn->setX509FingerprintMismatchAccept(true);
        }
        else {
            conn->setX509FingerprintMismatchAccept(false);
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
void MyFrameMain::conn_spawn(const wxString& service, int listenPort)
{
  wxLogDebug("MyFrameMain: %s: %s %d", __func__, service, listenPort);

  VNCConn* c = new VNCConn(this);

  /*
    Create UI elements
   */
  ViewerWindow* win = new ViewerWindow(notebook_connections, c);
  win->showStats(show_stats);
  win->showOneToOne(show_1to1);
#ifdef MULTIVNC_GRABKEYBOARD
  win->grabKeyboard(frame_main_toolbar->GetToolState(ID_GRABKEYBOARD));
#endif

  VNCSeamlessConnector* sc = 0;
  if(VNCSeamlessConnector::isSupportedByCurrentPlatform() && show_seamless != EDGE_NONE)
    sc = new VNCSeamlessConnector(this, c, show_seamless);

  /*
    save VNCConn and UI elements blobbed together
   */
  ConnBlob cb;
  cb.conn = c;
  cb.viewerwindow = win;
  cb.seamlessconnector = sc;
  cb.windowshare_proc = 0;
  cb.windowshare_proc_pid = 0;

  connections.push_back(cb);

  // enable "disconnect"
  frame_main_menubar->Enable(wxID_STOP, true);
  GetToolBar()->EnableTool(wxID_STOP, true);

  /*
    Initialise connection
   */
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

  if(listenPort > 0)
    {
      // add to notebook, needs to be after connections list add
      notebook_connections->AddPage(win, wxEmptyString, true);
      wxLogStatus(_("Listening on port") + " " + (wxString() << listenPort) + wxT(" ..."));
      c->Listen(listenPort);
    }
  else // normal init without previous listen
    {
      wxString user, host, ssh_user, ssh_host, ssh_fingerprint, ssh_priv_key_filename, x509_fingerprint;
      wxSecretValue password, ssh_password, ssh_priv_key_password;
      int repeaterId = -1, port = 5900, ssh_port = 22;

      wxString vncUriScheme = "vnc://";
      if (service.substr(0, vncUriScheme.length()) == vncUriScheme) {
          // vnc:// URI
          wxURI uri(service);
          host = uri.GetServer();
          uri.GetPort().ToInt(&port);
          user = getQueryValue(uri, "VncUsername"); // RFC 7869
          if (user.IsEmpty()) {
              user = uri.GetUserInfo(); // fallback
          }
          wxSecretString vncPassword = getQueryValue(uri, "VncPassword"); // RFC 7869
          if (!vncPassword.IsEmpty()) {
              password = wxSecretValue(vncPassword);
          }

          getQueryValue(uri, "RepeaterId").ToInt(&repeaterId);

          ssh_host = getQueryValue(uri, "SshHost");       // RFC 7869
          getQueryValue(uri, "SshPort").ToInt(&ssh_port); // RFC 7869

          ssh_fingerprint = getQueryValue(uri, "IdHash"); // RFC 7869 colon-seperated hex
          ssh_fingerprint.Replace(":", "");
          ssh_fingerprint.MakeLower();                    // now lowercase hex
          if (ssh_fingerprint.IsEmpty()) {
              // if no fingerprint given, use the one from our internal known_hosts
              ssh_fingerprint = ssh_known_hosts_load(ssh_host, wxString() << ssh_port);
          }

          ssh_user = getQueryValue(uri, "SshUsername");   // RFC 7869
          wxSecretString sshPassword = getQueryValue(uri, "SshPassword"); // RFC 7869
          if (!sshPassword.IsEmpty()) {
              ssh_password = wxSecretValue(sshPassword);
          }

          wxSecretString sshPrivKeyPassword = getQueryValue(uri, "SshPrivKeyPassword"); // not standardised
          if (!sshPrivKeyPassword.IsEmpty()) {
              ssh_priv_key_password = wxSecretValue(sshPrivKeyPassword);
          }

          ssh_priv_key_filename = getQueryValue(uri, "SshPrivKeyFilename"); // not standardised

          if(ssh_host.IsEmpty()) {
              // if not using SSH, use this for TLS
              x509_fingerprint = getQueryValue(uri, "IdHash"); // RFC 7869 colon-seperated hex
              x509_fingerprint.Replace(":", "");
              x509_fingerprint.MakeLower();                    // now lowercase hex
              if (x509_fingerprint.IsEmpty()) {
                  // if no fingerprint given, use the one from our internal known_hosts
                  x509_fingerprint = x509_known_hosts_load(uri.GetServer(), wxString() << port);
              }
          }
      } else {
          // user@host:port notation
          user = service.Contains("@") ? service.BeforeFirst('@') : "";
          char *tmp;
          parseHostString((service.Contains("@") ? service.AfterFirst('@') : service).mb_str(), 5900, &tmp, &port);
          host = wxString(tmp);
          free(tmp);

          // this notation cannot give a fingerprint, use the one from our internal known_hosts
          x509_fingerprint = x509_known_hosts_load(host, wxString() << port);
      }

      // add to notebook, needs to be after connections list add
      notebook_connections->AddPage(win, wxString::Format(_("Connecting to %s..."), host), true);
      wxLogStatus(_("Connecting to %s..."), host);

      if(!c->Init(host,
                  port,
                  repeaterId,
                  user,
                  password,
                  ssh_host,
                  ssh_port,
                  ssh_fingerprint,
                  ssh_user,
                  ssh_password,
                  ssh_priv_key_filename,
                  ssh_priv_key_password,
                  x509_fingerprint,
                  encodings, compresslevel, quality, multicast,
                  multicast_socketrecvbuf, multicast_recvbuf)) {
          wxLogError(c->getErr());
      }
    }
}


void MyFrameMain::conn_setup(VNCConn *c) {

  wxLogDebug("%s: %p", __func__, c);

  // first, find related ConnBlob
  ConnBlob *cb;
  vector<ConnBlob>::iterator it = connections.begin();
  size_t index = 0;
  while(it != connections.end() && it->conn != c)
      {
	  ++it;
	  ++index;
      }
  if (index < connections.size()) {
      cb = &*it;
  } else {
      wxLogFatalError("Could not find UI elements for connection %p. Please report as a bug", c);
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

  // set page label
  wxString label;
  if(!c->getListenPort().IsEmpty()) {
      label = c->getDesktopName() + " " + _("(Reverse Connection)");
  } else {
      if (c->getSshHost().IsEmpty()) {
          if (c->getRepeaterId().IsEmpty()) {
              label = wxString::Format("%s (%s)", c->getDesktopName(), c->getServerHost());
          } else {
              label = wxString::Format(_("%s (peer %s on %s)"), c->getDesktopName(), c->getRepeaterId(), c->getServerHost());
          }
      } else {
          if (c->getRepeaterId().IsEmpty()) {
              label = wxString::Format(_("%s (%s via %s)"), c->getDesktopName(), c->getServerHost(), c->getSshHost());
          } else {
              label = wxString::Format(_("%s (peer %s on %s via %s)"), c->getDesktopName(), c->getRepeaterId(), c->getServerHost(), c->getSshHost());
          }
      }
  }
  notebook_connections->SetPageText(index, label);


  if(c->isMulticast())
    notebook_connections->SetPageImage(index, 1);
  else
    notebook_connections->SetPageImage(index, 0);

  if(fastrequest)
    c->setFastRequest(fastrequest_interval);

  if(qos_ef)
    c->setDSCP(184); // 184 == 0xb8 == expedited forwarding


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
      GetToolBar()->EnableTool(wxID_SAVE, true); // screenshot
      GetToolBar()->EnableTool(ID_INPUT_REPLAY, true);
      GetToolBar()->EnableTool(ID_INPUT_RECORD, true);
    }

}


void MyFrameMain::conn_terminate(int which)
{
  if(which == wxNOT_FOUND)
    return;

  ConnBlob* cb = &connections.at(which);

  wxLogDebug("%s: %p at %d", __func__, cb->conn, which);

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
      wxLogDebug("conn_terminate(): trying to kill %d.", cb->windowshare_proc_pid);
      if(!wxProcess::Exists(cb->windowshare_proc_pid))
	{
	  wxLogDebug("conn_terminate(): window sharing helper PID does not exist!");
	}
      else
	{
	  // avoid callback call, now obj pointed to by windowshare_proc deletes itself!
	  cb->windowshare_proc->Detach(); 
	  if(wxKill(cb->windowshare_proc_pid, wxSIGTERM, NULL, wxKILL_CHILDREN) == 0)
	    wxLogDebug("conn_terminate(): successfully killed %d.", cb->windowshare_proc_pid);
	  else
	    wxLogDebug("conn_terminate(): could not kill %d.", cb->windowshare_proc_pid);
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
}


bool MyFrameMain::bookmarks_load_to_list()
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
  wxMenu* bm_menu = frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(_("Bookmarks")));
  for(int i = bm_menu->GetMenuItemCount()-1; i > 0; --i)
    bm_menu->Destroy(bm_menu->FindItemByPosition(i));
  bm_menu->AppendSeparator();


  // then read in each bookmark value pair
  for(size_t i=0; i < bookmarknames.GetCount(); ++i)
    {
      // and add to bookmarks menu
      int id = NewControlId();
      wxString* index_str = new wxString; // pack i into a wxObject, we use wxString here
      *index_str << i;
      bm_menu->Append(id, bookmarknames[i]);
      Connect(id, wxEVT_COMMAND_MENU_SELECTED, 
	      wxCommandEventHandler(FrameMain::listbox_bookmarks_dclick), (wxObject*)index_str);
     }
  
  cfg->SetPath(wxT("/"));

  list_box_bookmarks->Set(bookmarknames);

  return true;
}


wxSecretString MyFrameMain::bookmarks_load_one(int index) {
    wxConfigBase *cfg = wxConfigBase::Get();

    /*
      Find the subgroup at index
    */
    cfg->SetPath(G_BOOKMARKS);
    wxString str;
    long dummy;
    int i = 0;
    bool cont = cfg->GetFirstGroup(str, dummy);
    while(cont && i < index) {
        ++i;
        cont = cfg->GetNextGroup(str, dummy);
    }

    /*
      Read values from config
    */
    wxString host, port, user, ssh_host, ssh_user, ssh_port, ssh_priv_key_filename;
    cfg->SetPath(G_BOOKMARKS + str);

    if(!cfg->Read(K_BOOKMARKS_HOST, &host)) {
        wxLogError(_("Error reading hostname of bookmark '%s'!"), str);
        cfg->SetPath("/");
        return wxString();
    }

    if(!cfg->Read(K_BOOKMARKS_PORT, &port)) {
        wxLogError(_("Error reading port of bookmark '%s'!"), str);
        cfg->SetPath("/");
        return wxString();
    }

    // user and SSH are optional
    cfg->Read(K_BOOKMARKS_USER, &user);
    cfg->Read(K_BOOKMARKS_SSH_HOST, &ssh_host);
    cfg->Read(K_BOOKMARKS_SSH_USER, &ssh_user);
    cfg->Read(K_BOOKMARKS_SSH_PORT, &ssh_port);
    cfg->Read(K_BOOKMARKS_SSH_PRIV_KEY_FILENAME, &ssh_priv_key_filename);

    // done reading cfg
    cfg->SetPath("/");

    // add brackets if host is an IPv6 address
    if(host.Freq(':') > 0) {
        host = wxT("[") + host + wxT("]");
    }

    /*
      Read values from secretstore
    */
    wxSecretValue password, sshPassword, sshPrivKeyPassword;
    bookmarks_secrets_load(str, password, sshPassword, sshPrivKeyPassword);

    wxSecretString uri("vnc://");
    uri += (host + ":" + port);
    uri += ("?VncUsername=" + user);
    uri += ("&VncPassword=" + password.GetAsString());
    uri += ("&SshHost=" + ssh_host);
    uri += ("&SshPort=" + ssh_port);
    uri += ("&SshUsername=" + ssh_user);
    uri += ("&SshPassword=" + sshPassword.GetAsString());
    uri += ("&SshPrivKeyFilename=" + ssh_priv_key_filename);
    uri += ("&SshPrivKeyPassword=" + sshPrivKeyPassword.GetAsString());

    return uri;
}


wxString MyFrameMain::getQueryValue(const wxURI& wxUri, const wxString& key) {
    wxString query = wxUri.GetQuery(); // Get everything after "?"

    if (query.IsEmpty())
        return wxEmptyString;

    // Split into key-value pairs
    wxStringTokenizer tokenizer(query, "&");
    while (tokenizer.HasMoreTokens())
    {
        wxString pair = tokenizer.GetNextToken();
        wxArrayString keyValue = wxSplit(pair, '=');
        if (keyValue.size() == 2 && keyValue[0] == key)
        {
            return keyValue[1];
        }
    }
    return wxEmptyString; // Key not found
}


/*
  parse ipv4 or ipv6 address string.
  taken from remmina, thanks!
*/
void MyFrameMain::parseHostString(const char *server, int defaultport, char **host, int *port)
{
	char *str, *ptr, *ptr2;

	str = strdup(server);

	/* [server]:port format */
	ptr = strchr(str, '[');
	if (ptr)
	{
		ptr++;
		ptr2 = strchr(ptr, ']');
		if (ptr2)
			*ptr2++ = '\0';
		if (*ptr2 == ':')
			defaultport = atoi(ptr2 + 1);
		if (host)
			*host = strdup(ptr);
		if (port)
			*port = defaultport;
		free(str);
		return;
	}

	/* server:port format, IPv6 cannot use this format */
	ptr = strchr(str, ':');
	if (ptr)
	{
		ptr2 = strchr(ptr + 1, ':');
		if (ptr2 == NULL)
		{
			*ptr++ = '\0';
			defaultport = atoi(ptr);
		}
		/* More than one ':' means this is IPv6 address. Treat it as a whole address */
	}
	if (host)
		*host = str;
	if (port)
		*port = defaultport;
}


/*
  public members
*/


void MyFrameMain::machine_connect(wxCommandEvent &event)
{
    wxConfigBase *pConfig = wxConfigBase::Get();
    wxString server, port;
    pConfig->Read(K_LASTSERVER, &server);
    pConfig->Read(K_LASTPORT, &port);

    MyDialogNewConnection dialog_new_connection(this, wxID_ANY, _("Connect to a specific host."));
    dialog_new_connection.setVncServer(server);
    dialog_new_connection.setVncPort(port);
    dialog_new_connection.setShowAdvanced(pConfig->ReadBool(K_NEW_CONN_SHOW_ADVANCED, V_NEW_CONN_SHOW_ADVANCED));

    if(dialog_new_connection.ShowModal() == wxID_OK && dialog_new_connection.getVncServer() != wxEmptyString) {
        pConfig->Write(K_LASTSERVER, dialog_new_connection.getVncServer());
        pConfig->Write(K_LASTPORT, dialog_new_connection.getVncPort());
        wxString server = dialog_new_connection.getVncServer();
        if (server.Contains(":")) {
            // IPv6 address, need brackets as per RFC 2732
            server = "[" + server + "]";
        }
        conn_spawn("vnc://" + server
                   + wxString(dialog_new_connection.getVncPort().IsEmpty() ? "" : ":" + dialog_new_connection.getVncPort())
                   + "?RepeaterId=" + wxString::Format("%i", dialog_new_connection.getRepeaterId())
                   + "&SshHost=" + dialog_new_connection.getSshServer()
                   + "&SshPort=" + dialog_new_connection.getSshPort()
                   + "&SshUsername=" + dialog_new_connection.getSshUser()
                   + "&SshPassword=" + dialog_new_connection.getSshPassword().GetAsString()
                   + "&SshPrivKeyFilename=" + dialog_new_connection.getSshPrivKeyFilename()
                   + "&SshPrivKeyPassword=" + dialog_new_connection.getSshPrivkeyPassword().GetAsString()
                   , -1);
    }

    // save this in any case
    pConfig->Write(K_NEW_CONN_SHOW_ADVANCED, dialog_new_connection.getShowAdvanced());
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
  conn_spawn(wxEmptyString, port);
}



void MyFrameMain::machine_disconnect(wxCommandEvent &event)
{
    // shut down connection thats currently selected
    int sel;
    if((sel = notebook_connections->GetSelection()) != -1) {
        connections.at(sel).conn->Shutdown();
    }
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
  name.Trim();

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
      cfg->Write(K_BOOKMARKS_SSH_HOST, c->getSshHost());
      cfg->Write(K_BOOKMARKS_SSH_PORT, c->getSshPort());
      cfg->Write(K_BOOKMARKS_SSH_USER, c->getSshUserName());
      cfg->Write( K_BOOKMARKS_SSH_PRIV_KEY_FILENAME, c->getSshPrivKeyFilename());

      bookmarks_secrets_save(name, c->getPassword(), c->getSshPassword(), c->getSshPrivKeyPassword());

      //reset path
      cfg->SetPath(wxT("/"));

      // and load into listbox
      bookmarks_load_to_list();
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
  newname.Trim();
  if(newname.IsEmpty())
    return;
    
  wxConfigBase *cfg = wxConfigBase::Get();

  wxSecretValue password, sshPassword, sshPrivKeyPassword;
  bookmarks_secrets_load(sel, password, sshPassword, sshPrivKeyPassword);
  bookmarks_secrets_delete(sel);

  cfg->SetPath(G_BOOKMARKS);
  cfg->RenameGroup(sel, newname);
  //reset path
  cfg->SetPath(wxT("/"));

  bookmarks_secrets_save(newname, password, sshPassword, sshPrivKeyPassword);

  // and load into listbox
  bookmarks_load_to_list();
}



void MyFrameMain::bookmarks_delete(wxCommandEvent &event)
{
  wxString name = list_box_bookmarks->GetStringSelection();
  
  if(name.IsEmpty())
    {
      wxLogError(_("No bookmark selected!"));
      return;
    }

  bookmarks_secrets_delete(name);

  wxConfigBase *cfg = wxConfigBase::Get();
  if(!cfg->DeleteGroup(G_BOOKMARKS + name))
    wxLogError(_("No bookmark with this name!"));

  // and re-read
  bookmarks_load_to_list();
}


void MyFrameMain::bookmarks_secrets_save(const wxString& bookmarkName,
                              const wxSecretValue& password,
                              const wxSecretValue& sshPassword,
                              const wxSecretValue& sshPrivKeyPassword) {
#if wxUSE_SECRETSTORE
    wxSecretStore store = wxSecretStore::GetDefault();
    if (store.IsOk()) {
        if(password.IsOk()) {
            if(!store.Save("MultiVNC/Bookmarks/" + bookmarkName + " VncPassword", wxEmptyString, password)) {
                wxLogWarning(_("Failed to save credentials to the system secret store."));
            }
        }

        if(sshPassword.IsOk()) {
            if(!store.Save("MultiVNC/Bookmarks/" + bookmarkName + " SshPassword", wxEmptyString, sshPassword)) {
                    wxLogWarning(_("Failed to save credentials to the system secret store."));
            }
        }

        if(sshPrivKeyPassword.IsOk()) {
            if(!store.Save("MultiVNC/Bookmarks/" + bookmarkName + " SshPrivKeyPassword", wxEmptyString, sshPrivKeyPassword)) {
                wxLogWarning(_("Failed to save credentials to the system secret store."));
            }
        }
    }
#endif
}


void MyFrameMain::bookmarks_secrets_load(const wxString& bookmarkName,
                              wxSecretValue& password,
                              wxSecretValue& sshPassword,
                              wxSecretValue& sshPrivKeyPassword) {
    /*
       Read stuff for pre-0.11 versions TODO remove with 0.12
     */
    wxConfigBase *cfg = wxConfigBase::Get();
    wxString host, port, user;
    cfg->SetPath(G_BOOKMARKS + bookmarkName);
    if(!cfg->Read(K_BOOKMARKS_HOST, &host)) {
        wxLogError(_("Error reading hostname of bookmark '%s'!"), bookmarkName);
        cfg->SetPath("/");
        return;
    }
    if(!cfg->Read(K_BOOKMARKS_PORT, &port)) {
        wxLogError(_("Error reading port of bookmark '%s'!"), bookmarkName);
        cfg->SetPath("/");
        return;
    }
    // user is optional
    cfg->Read(K_BOOKMARKS_USER, &user);
    // done reading cfg
    cfg->SetPath("/");

#if wxUSE_SECRETSTORE
    wxSecretStore store = wxSecretStore::GetDefault();
    if (store.IsOk()) {
        wxString username; // this will not be used
        store.Load("MultiVNC/Bookmarks/" + bookmarkName + " VncPassword", username, password);
        store.Load("MultiVNC/Bookmarks/" + bookmarkName + " SshPassword", username, sshPassword);
        store.Load("MultiVNC/Bookmarks/" + bookmarkName + " SshPrivKeyPassword", username, sshPrivKeyPassword);

        // still load saves from pre-0.11 versions TODO remove with 0.12
        if (!password.IsOk()) {
            store.Load("MultiVNC/Bookmarks/" + wxString(user.IsEmpty() ? "" : user + "@") + host + ":" + port,
                       username,
                       password); // if Load() fails, password will still be empty
            if (password.IsOk()) {
                // side effect: if there was a password from the legacy location, move it to the new one
                store.Delete("MultiVNC/Bookmarks/" + wxString(user.IsEmpty() ? "" : user + "@") + host + ":" + port);
                store.Save("MultiVNC/Bookmarks/" + bookmarkName + " VncPassword", wxEmptyString, password);
            }
        }
    }
#endif

}

void MyFrameMain::bookmarks_secrets_delete(const wxString& bookmarkName) {
    /*
       Read stuff for pre-0.11 versions TODO remove with 0.12
     */
    wxConfigBase *cfg = wxConfigBase::Get();
    wxString host, port, user;
    cfg->SetPath(G_BOOKMARKS + bookmarkName);
    if(!cfg->Read(K_BOOKMARKS_HOST, &host)) {
        wxLogError(_("Error reading hostname of bookmark '%s'!"), bookmarkName);
        cfg->SetPath("/");
        return;
    }
    if(!cfg->Read(K_BOOKMARKS_PORT, &port)) {
        wxLogError(_("Error reading port of bookmark '%s'!"), bookmarkName);
        cfg->SetPath("/");
        return;
    }
    // user is optional
    cfg->Read(K_BOOKMARKS_USER, &user);
    // done reading cfg
    cfg->SetPath("/");

#if wxUSE_SECRETSTORE
    wxSecretStore store = wxSecretStore::GetDefault();
    if (store.IsOk()) {
        store.Delete("MultiVNC/Bookmarks/" + bookmarkName + " VncPassword");
        store.Delete("MultiVNC/Bookmarks/" + bookmarkName + " SshPassword");
        store.Delete("MultiVNC/Bookmarks/" + bookmarkName + " SshPrivKeyPassword");
        // still delete saves from pre-0.11 versions TODO remove with 0.12
        store.Delete("MultiVNC/Bookmarks/" + wxString(user.IsEmpty() ? "" : user + "@") + host + ":" + port);
    }
#endif
}


void MyFrameMain::ssh_known_hosts_save(const wxString& host, const wxString& port, const wxString& fingerprint) {
    wxConfigBase *cfg = wxConfigBase::Get();
    wxString name = host + ":" + port;
    if(name != wxEmptyString) {
        cfg->SetPath(G_SSH_KNOWN_HOSTS + name);
        cfg->Write(K_SSH_KNOWN_HOSTS_FINGERPRINT, fingerprint);
        //reset path
        cfg->SetPath("/");
    }
}


wxString MyFrameMain::ssh_known_hosts_load(const wxString& host, const wxString& port) {
    wxString fingerprint;
    wxConfigBase *cfg = wxConfigBase::Get();
    wxString name = host + ":" + port;
    cfg->SetPath(G_SSH_KNOWN_HOSTS + name);
    cfg->Read(K_SSH_KNOWN_HOSTS_FINGERPRINT, &fingerprint);
    //reset path
    cfg->SetPath("/");
    return fingerprint;
}

wxString MyFrameMain::hex_to_base64(const wxString& hex) {
    if (hex.size() % 2 != 0) {
        return wxEmptyString;
    }

    std::vector<uint8_t> bytes;
    bytes.reserve(hex.size() / 2);

    for (size_t i = 0; i < hex.size(); i += 2) {
        int raw = wxHexToDec(hex.substr(i, 2).MakeUpper());
        if (raw < 0) { // -1 if input is not hex
            return wxEmptyString;
        }
        bytes.push_back(raw);
    }

    wxString base64 = wxBase64Encode(bytes.data(), bytes.size());
    base64.Replace("=", ""); // remove padding
    return base64;
}

void MyFrameMain::x509_known_hosts_save(const wxString& host, const wxString& port, const wxString& fingerprint) {
    wxConfigBase *cfg = wxConfigBase::Get();
    wxString name = host + ":" + port;
    if(name != wxEmptyString) {
        cfg->SetPath(G_X509_KNOWN_HOSTS + name);
        cfg->Write(K_X509_KNOWN_HOSTS_FINGERPRINT, fingerprint);
        //reset path
        cfg->SetPath("/");
    }
}


wxString MyFrameMain::x509_known_hosts_load(const wxString& host, const wxString& port) {
    wxString fingerprint;
    wxConfigBase *cfg = wxConfigBase::Get();
    wxString name = host + ":" + port;
    cfg->SetPath(G_X509_KNOWN_HOSTS + name);
    cfg->Read(K_X509_KNOWN_HOSTS_FINGERPRINT, &fingerprint);
    //reset path
    cfg->SetPath("/");
    return fingerprint;
}

wxString MyFrameMain::hex_to_colon_separated(const wxString& hex) {
    wxString result;
    for (size_t i = 0; i < hex.Length(); i += 2) {
        if (i != 0) result += ":";
        result += hex.Mid(i, 2).Upper();
    }
    return result;
}

void MyFrameMain::help_about(wxCommandEvent &event)
{	
  wxAboutDialogInfo info;
  wxIcon icon = bitmapBundleFromSVGResource("about").GetIcon(this->FromDIP(wxSize(128,128)));

  wxString desc = "\n";
  desc += _("MultiVNC is a cross-platform SSH-enabled VNC client.");
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


  conn_spawn(services_addr + wxT(":") + services_port);
} 
 


void MyFrameMain::listbox_bookmarks_select(wxCommandEvent &event)
{
  int sel = event.GetInt();

  if(sel >= 0) // something selected
    {
      wxLogStatus(_("Bookmark %s"), list_box_bookmarks->GetString(sel));
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

  conn_spawn(bookmarks_load_one(sel));
}


void MyFrameMain::listbox_bookmarks_context(wxContextMenuEvent &event)
{
    wxPoint clientPosition =
        list_box_bookmarks->ScreenToClient(event.GetPosition());

    int itemIndex = list_box_bookmarks->HitTest(clientPosition);

    if (itemIndex != wxNOT_FOUND) {
        // Programmatically select the item. This is for UX purposes and,
        // more important, so that edit and delete function work on the
        // correct item.
        list_box_bookmarks->SetSelection(itemIndex);

        // and show the context menu
        wxMenu menu;
        menu.Append(wxID_EDIT, _("&Edit Bookmark"));
        menu.Bind(wxEVT_MENU, &MyFrameMain::bookmarks_edit, this, wxID_EDIT);
        menu.Append(wxID_DELETE, _("&Delete Bookmark"));
        menu.Bind(wxEVT_MENU, &MyFrameMain::bookmarks_delete, this, wxID_DELETE);
        PopupMenu(&menu);
    }
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



void MyFrameMain::cmdline_conn_spawn(const wxString& service, int listenPort)
{
    conn_spawn(service, listenPort);
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




