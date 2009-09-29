
#include <cstdarg>
#include <csignal>
#include "wx/intl.h"
#include "wx/log.h"
#include "wx/thread.h"
#include "wx/socket.h"

#include "VNCConn.h"




// use some global address
#define VNCCONN_OBJ_ID (void*)VNCConn::got_update

// logfile name
#define LOGFILE _T("MultiVNC.log")


// define our new notify events!
DEFINE_EVENT_TYPE(VNCConnDisconnectNOTIFY)
DEFINE_EVENT_TYPE(VNCConnUpdateNOTIFY)

// pixelformat defaults
// seems 8,3,4 and 5,3,2 are possible with rfbGetClient()
#define BITSPERSAMPLE 8
#define SAMPLESPERPIXEL 3
#define BYTESPERPIXEL 4 





/********************************************

  internal worker thread class

********************************************/

class VNCThread : public wxThread
{
  VNCConn *p;    // our parent object
 
public:
  VNCThread(VNCConn *parent);
  
  // thread execution starts here
  virtual wxThread::ExitCode Entry();
  
  // called when the thread exits - whether it terminates normally or is
  // stopped with Delete() (but not when it is Kill()ed!)
  virtual void OnExit();
};





VNCThread::VNCThread(VNCConn *parent)
  : wxThread()
{
  p = parent;
}



wxThread::ExitCode VNCThread::Entry()
{
  int i;

#ifdef __WXGTK__
  // this signal is generated when we pop up a file dialog wwith wxGTK
  // we need to block it here cause it interrupts the select() call
  // in WaitForMessage()
  sigset_t            newsigs;
  sigset_t            oldsigs;
  sigemptyset(&newsigs);
  sigemptyset(&oldsigs);
  sigaddset(&newsigs, SIGRTMIN-1);
#endif

  while(! TestDestroy()) 
    {
#ifdef __WXGTK__
      sigprocmask(SIG_BLOCK, &newsigs, &oldsigs);
#endif

      i=WaitForMessage(p->cl, 500);

#ifdef __WXGTK__
      sigprocmask(SIG_SETMASK, &oldsigs, NULL);
#endif

      if(i<0)
	{
	  wxLogDebug(wxT("VNCConn %p: vncthread waitforMessage() failed"), p);
	  p->SendDisconnectNotify();
	  return 0;
	}
      if(i)
	if(!HandleRFBServerMessage(p->cl))
	  {
	    wxLogDebug(wxT("VNCConn %p: vncthread HandleRFBServerMessage() failed"), p);
	    p->SendDisconnectNotify();
	    return 0;
	  }
     
    }
}



void VNCThread::OnExit()
{
  // cause wxThreads delete themselves after completion
  p->vncthread = 0;
  wxLogDebug(wxT("VNCConn %p: vncthread exiting"), p);
}






/********************************************

  VNCConn class

********************************************/

/*
  constructor/destructor
*/


VNCConn::VNCConn(void* p)
{
  // save our caller
  parent = p;
  
  vncthread = 0;
  cl = 0;
  framebuffer = 0;
  fb_data = 0;

  rfbClientLog = rfbClientErr = logger;
}




VNCConn::~VNCConn()
{
  Shutdown();
}



/*
  private members
*/


void VNCConn::SendDisconnectNotify() 
{
  wxLogDebug(wxT("VNCConn %p: SendDisconnectNotify()"), this);

  // new NOTIFY event, we got no window id
  wxCommandEvent event(VNCConnDisconnectNOTIFY, wxID_ANY);
  event.SetEventObject(this); // set sender

  // Send it
  wxPostEvent((wxEvtHandler*)parent, event);
}



void VNCConn::SendUpdateNotify(int x, int y, int w, int h)
{
  // new NOTIFY event, we got no window id
  wxCommandEvent event(VNCConnUpdateNOTIFY, wxID_ANY);
  event.SetEventObject(this); // set sender

  // set info about what was updated
  wxRect* rect = new wxRect(x, y, w, h);
  event.SetClientData(rect);
  wxLogDebug(wxT("VNCConn %p: SendUpdateNotify(%i,%i,%i,%i)"), this,
	     rect->x,
	     rect->y,
	     rect->width,
	     rect->height);

  // Send it
  wxPostEvent((wxEvtHandler*)parent, event);
}





rfbBool VNCConn::alloc_framebuffer(rfbClient* client)
{
  // get VNCConn object belonging to this client
  VNCConn* conn = (VNCConn*) rfbClientGetClientData(client, VNCCONN_OBJ_ID); 

  // assert 32bpp, as requested with GetClient() in Init()
  if(client->format.bitsPerPixel != 32)
    {
      conn->err.Printf(_("Failure setting up framebuffer: wrong BPP!"));
      return false;
    }


  // setup framebuffer
  if(conn->framebuffer)
    {
      if(conn->fb_data && client->frameBuffer)
	conn->framebuffer->UngetRawData(*conn->fb_data);    

      delete conn->framebuffer;
    }
  conn->framebuffer = new wxBitmap(client->width, client->height, client->format.bitsPerPixel);
  

  if(conn->fb_data)
    delete conn->fb_data;
  conn->fb_data = new wxAlphaPixelData(*conn->framebuffer);
  if ( ! *conn->fb_data )
    {
      conn->err.Printf(_("Failed to gain raw access to framebuffer data!"));
      delete conn->framebuffer;
      return false;
    }

  // zero it out
  wxAlphaPixelData::Iterator fb_it(*conn->fb_data);
  for ( int y = 0; y < client->height; ++y )
    {
      wxAlphaPixelData::Iterator rowStart = fb_it;

      for ( int x = 0; x < client->width; ++x, ++fb_it )
	{
	  fb_it.Red() = 0;
	  fb_it.Green() = 0;
	  fb_it.Blue() = 0;
	  fb_it.Alpha() = 255; // 255 is opaque
	}
      
      fb_it = rowStart;
      fb_it.OffsetY(*conn->fb_data, 1);
    }


  // connect the wxBitmap buffer to client's framebuffer so we write directly into
  // the wxBitmap
  // GetRawData returns NULL on failure
  client->frameBuffer = (uint8_t*) conn->framebuffer->GetRawData(*conn->fb_data, client->format.bitsPerPixel);
#ifdef __WIN32__
  // under windows, the returned pointer points to the _end_ of the raw data,
  // as windows DIBs are stored bottom to top, so rewind it.
  // we end up with an upside down image, but later correct that in getFrameBufferRegion()...
  if ( client->height > 1 )
    client->frameBuffer -= (client->height - 1)* -conn->fb_data->m_stride;
#endif
  
  return client->frameBuffer ? TRUE : FALSE;
}





void VNCConn::got_update(rfbClient* client,int x,int y,int w,int h)
{
  VNCConn* conn = (VNCConn*) rfbClientGetClientData(client, VNCCONN_OBJ_ID); 
 
  conn->SendUpdateNotify(x, y, w, h);
}





void VNCConn::kbd_leds(rfbClient* cl, int value, int pad)
{
  VNCConn* conn = (VNCConn*) rfbClientGetClientData(cl, VNCCONN_OBJ_ID); 
  wxLogDebug(wxT("VNCConn %p: Led State= 0x%02X\n"), conn, value);
}


void VNCConn::textchat(rfbClient* cl, int value, char *text)
{
  VNCConn* conn = (VNCConn*) rfbClientGetClientData(cl, VNCCONN_OBJ_ID); 
}


void VNCConn::got_selection(rfbClient *cl, const char *text, int len)
{
  VNCConn* conn = (VNCConn*) rfbClientGetClientData(cl, VNCCONN_OBJ_ID); 
}



// there's no per-connection log since we cannot find out which client
// called the logger function :-(
wxArrayString VNCConn::log;
wxCriticalSection VNCConn::mutex_log;

void VNCConn::logger(const char *format, ...)
{
  FILE* logfile;
  wxString logfile_str = LOGFILE;
  va_list args;
  wxChar timebuf[256];
  time_t log_clock;
  wxString wx_format(format, wxConvUTF8);

  if(!rfbEnableClientLogging)
    return;

  // since we're accessing some global things here from different threads
  wxCriticalSectionLocker lock(mutex_log);

  // delete logfile on program startup
  static bool firstrun = 1;
  if(firstrun)
    {
      remove(logfile_str.char_str());
      firstrun = 0;
    }

  logfile=fopen(logfile_str.char_str(),"a");

  time(&log_clock);
  wxStrftime(timebuf, WXSIZEOF(timebuf), _T("%d/%m/%Y %X "), localtime(&log_clock));

  // global log string array
  va_start(args, format);
  char msg[1024];
  vsnprintf(msg, 1024, format, args);
  log.Add( wxString(timebuf) + wxString(msg, wxConvUTF8));
  va_end(args);

  // global log file
  va_start(args, format);
  fprintf(logfile, wxString(timebuf).mb_str());
  vfprintf(logfile, format, args);
  va_end(args);
 
  // and stderr
  va_start(args, format);
  fprintf(stderr, wxString(timebuf).mb_str());
  vfprintf(stderr, format, args);
  va_end(args);
  
  
  fclose(logfile);
}






/*
  public members
*/


bool VNCConn::Init(const wxString& host, char* (*getpasswdfunc)(rfbClient*), int compresslevel, int quality)
{
  Shutdown();

  int argc = 6;
  char* argv[argc];
  argv[0] = strdup("VNCConn");
  argv[1] = strdup(host.mb_str());
  argv[2] = strdup("-compress");
  argv[3] = strdup((wxString() << compresslevel).mb_str());
  argv[4] = strdup("-quality");
  argv[5] = strdup((wxString() << quality).mb_str());
  
   
  // this takes (int bitsPerSample,int samplesPerPixel, int bytesPerPixel) 
  // 5,3,2 and 8,3,4 seem possible
  cl=rfbGetClient(BITSPERSAMPLE, SAMPLESPERPIXEL, BYTESPERPIXEL);

  rfbClientSetClientData(cl, VNCCONN_OBJ_ID, this); 
  
  // callbacks
  cl->MallocFrameBuffer = alloc_framebuffer;
  cl->GotFrameBufferUpdate = got_update;
  cl->GetPassword = getpasswdfunc;
  cl->HandleKeyboardLedState = kbd_leds;
  cl->HandleTextChat = textchat;
  cl->GotXCutText = got_selection;

  cl->canHandleNewFBSize = TRUE;
  
  if(! rfbInitClient(cl, &argc, argv))
    {
      cl = 0; //  rfbInitClient() calls rfbClientCleanup() on failure, but this does not zero the ptr
      err.Printf(_("Failure connecting to server!"));
      Shutdown();
      return false;
    }
  
  // if there was an error in alloc_framebuffer(), catch that here
  // err is set by alloc_framebuffer()
  if(! cl->frameBuffer)
    {
      Shutdown();
      return false;
    }
  

  // this is like our main loop
  VNCThread *tp = new VNCThread(this);
  // save it for later on
  vncthread = tp;
  
  if( tp->Create() != wxTHREAD_NO_ERROR )
    {
      err.Printf(_("Could not create VNC thread!"));
      Shutdown();
      return false;
    }

  if( tp->Run() != wxTHREAD_NO_ERROR )
    {
      err.Printf(_("Could not start VNC thread!")); 
      Shutdown();
      return false;
    }
   
  return true;
}




bool VNCConn::Shutdown()
{
  wxLogDebug(wxT("VNCConn %p: Shutdown()"), this);

  if(vncthread)
    {
      //wxCriticalSectionLocker lock(mutex_vncthread);
      wxLogDebug(wxT( "VNCConn %p: Shutdown() before vncthread delete"), this);
      ((VNCThread*)vncthread)->Delete();
      // wait for deletion to finish
      while(vncthread)
	wxMilliSleep(100);
      
      wxLogDebug(wxT("VNCConn %p: Shutdown() after vncthread delete"), this);
    }
  
  if(cl)
    {
      wxLogDebug(wxT( "VNCConn %p: Shutdown() before client cleanup"), this);
      rfbClientCleanup(cl);
      cl = 0;
      wxLogDebug(wxT( "VNCConn %p: Shutdown() after client cleanup"), this);
    }

  if(fb_data)
    {
      delete fb_data;
      fb_data = 0;
    }
 
  if(framebuffer)
    {
      delete framebuffer;
      framebuffer = 0;
    }
}




bool VNCConn::sendPointerEvent(wxMouseEvent &event)
{
  int buttonmask = 0;

  if(event.LeftIsDown())
    buttonmask |= rfbButton1Mask;

  if(event.MiddleIsDown())
    buttonmask |= rfbButton2Mask;
  
  if(event.RightIsDown())
    buttonmask |= rfbButton3Mask;

  if(event.GetWheelRotation() > 0)
    buttonmask |= rfbWheelUpMask;

  if(event.GetWheelRotation() < 0)
    buttonmask |= rfbWheelDownMask;

  wxLogDebug(wxT("VNCConn %p: sending pointer event at (%d,%d), buttonmask %d"), this, event.m_x, event.m_y, buttonmask);

  return SendPointerEvent(cl, event.m_x, event.m_y, buttonmask);
}



bool VNCConn::sendKeyEvent(wxKeyEvent &event, bool down, bool isChar)
{
  rfbKeySym k = 0;

  if(isChar)
    {
      // get tranlated char
      k = event.GetUnicodeKey();

      wxLogDebug(wxT("VNCConn %p: sending CHAR key event"), this);
      wxLogDebug(wxT("VNCConn %p:         wxkeycode:      %d"), this, event.GetKeyCode());
      wxLogDebug(wxT("VNCConn %p:         wxkeycode char: %c"), this, event.GetKeyCode());
      wxLogDebug(wxT("VNCConn %p:         unicode:        %d"), this, event.GetUnicodeKey());
      wxLogDebug(wxT("VNCConn %p:         unicode char:   %c"), this, event.GetUnicodeKey()); 
      wxLogDebug(wxT("VNCConn %p:         rfbkeysym:      0x%.3x down"), this, k);
      wxLogDebug(wxT("VNCConn %p:         rfbkeysym:      0x%.3x  up"), this, k);

      // down, then up
      SendKeyEvent(cl, k, true);
      SendKeyEvent(cl, k, false);
      return true;
    }
  else
    {
      // lookup k
      switch(event.GetKeyCode()) 
	{
	case WXK_BACK: k = XK_BackSpace; break;
	case WXK_TAB: k = XK_Tab; break;
	case WXK_CLEAR: k = XK_Clear; break;
	case WXK_RETURN: k = XK_Return; break;
	case WXK_PAUSE: k = XK_Pause; break;
	case WXK_ESCAPE: k = XK_Escape; break;
	case WXK_SPACE: k = XK_space; break;
	case WXK_DELETE: k = XK_Delete; break;
	case WXK_NUMPAD0: k = XK_KP_0; break;
	case WXK_NUMPAD1: k = XK_KP_1; break;
	case WXK_NUMPAD2: k = XK_KP_2; break;
	case WXK_NUMPAD3: k = XK_KP_3; break;
	case WXK_NUMPAD4: k = XK_KP_4; break;
	case WXK_NUMPAD5: k = XK_KP_5; break;
	case WXK_NUMPAD6: k = XK_KP_6; break;
	case WXK_NUMPAD7: k = XK_KP_7; break;
	case WXK_NUMPAD8: k = XK_KP_8; break;
	case WXK_NUMPAD9: k = XK_KP_9; break;
	case WXK_NUMPAD_DECIMAL: k = XK_KP_Decimal; break;
	case WXK_NUMPAD_DIVIDE: k = XK_KP_Divide; break;
	case WXK_NUMPAD_MULTIPLY: k = XK_KP_Multiply; break;
	case WXK_NUMPAD_SUBTRACT: k = XK_KP_Subtract; break;
	case WXK_NUMPAD_ADD: k = XK_KP_Add; break;
	case WXK_NUMPAD_ENTER: k = XK_KP_Enter; break;
	case WXK_NUMPAD_EQUAL: k = XK_KP_Equal; break;
	case WXK_UP: k = XK_Up; break;
	case WXK_DOWN: k = XK_Down; break;
	case WXK_RIGHT: k = XK_Right; break;
	case WXK_LEFT: k = XK_Left; break;
	case WXK_INSERT: k = XK_Insert; break;
	case WXK_HOME: k = XK_Home; break;
	case WXK_END: k = XK_End; break;
	case WXK_PAGEUP: k = XK_Page_Up; break;
	case WXK_PAGEDOWN: k = XK_Page_Down; break;
	case WXK_F1: k = XK_F1; break;
	case WXK_F2: k = XK_F2; break;
	case WXK_F3: k = XK_F3; break;
	case WXK_F4: k = XK_F4; break;
	case WXK_F5: k = XK_F5; break;
	case WXK_F6: k = XK_F6; break;
	case WXK_F7: k = XK_F7; break;
	case WXK_F8: k = XK_F8; break;
	case WXK_F9: k = XK_F9; break;
	case WXK_F10: k = XK_F10; break;
	case WXK_F11: k = XK_F11; break;
	case WXK_F12: k = XK_F12; break;
	case WXK_F13: k = XK_F13; break;
	case WXK_F14: k = XK_F14; break;
	case WXK_F15: k = XK_F15; break;
	case WXK_NUMLOCK: k = XK_Num_Lock; break;
	case WXK_CAPITAL: k = XK_Caps_Lock; break;
	case WXK_SCROLL: k = XK_Scroll_Lock; break;
	  //case WXK_RSHIFT: k = XK_Shift_R; break;
	case WXK_SHIFT: k = XK_Shift_L; break;
	  //case WXK_RCTRL: k = XK_Control_R; break;
	case WXK_CONTROL: k = XK_Control_L; break;
	  //    case WXK_RALT: k = XK_Alt_R; break;
	case WXK_ALT: k = XK_Alt_L; break;
	  // case WXK_RMETA: k = XK_Meta_R; break;
	  // case WXK_META: k = XK_Meta_L; break;
	case WXK_WINDOWS_LEFT: k = XK_Super_L; break;	
	case WXK_WINDOWS_RIGHT: k = XK_Super_R; break;	
	  //case WXK_COMPOSE: k = XK_Compose; break;
	  //case WXK_MODE: k = XK_Mode_switch; break;
	case WXK_HELP: k = XK_Help; break;
	case WXK_PRINT: k = XK_Print; break;
	  //case WXK_SYSREQ: k = XK_Sys_Req; break;
	case WXK_CANCEL: k = XK_Break; break;
	default: break;
	}

      if(k)
	{
	  wxLogDebug(wxT("VNCConn %p: sending key %s event:"), this, down ? wxT("down") : wxT("up"));
	  wxLogDebug(wxT("VNCConn %p:         wxkeycode:      %d"), this, event.GetKeyCode());
	  wxLogDebug(wxT("VNCConn %p:         wxkeycode char: %c"), this, event.GetKeyCode());
	  wxLogDebug(wxT("VNCConn %p:         unicode:        %d"), this, event.GetUnicodeKey());
	  wxLogDebug(wxT("VNCConn %p:         unicode char:   %c"), this, event.GetUnicodeKey()); 
	  wxLogDebug(wxT("VNCConn %p:         rfbkeysym:      0x%.3x %s"), this, k, down ? wxT("down") : wxT("up"));

	  return SendKeyEvent(cl, k, down);
	}
      else
	{
	  // nothing of the above?
	  // then propagate this event through the EVT_CHAR handler
	  event.Skip();
	  return false;
	}
    }

  wxLogDebug(wxT("VNCConn %p: no matching keysym found"), this);
  return false;
}



wxBitmap VNCConn::getFrameBufferRegion(const wxRect& rect) const
{
#ifdef __WXDEBUG__
  // save a picture only if the last is older than 5 seconds 
  static time_t t0 = 0,t1;
  t1 = time(NULL);
  if(t1 - t0 > 5)
    {
      t0 = t1;
      wxString fbdump( wxT("fb-dump-") + getServerName() + wxT("-") + getServerPort() + wxT(".bmp"));
      framebuffer->SaveFile(fbdump, wxBITMAP_TYPE_BMP);
      wxLogDebug(wxT("VNCConn %p: saved raw framebuffer to '%s'"), this, fbdump.c_str());
    }
#endif

  
  // don't use getsubbitmap() here, instead
  // copy directly from framebuffer into a new bitmap,
  // saving one complete copy iteration
  wxBitmap region(rect.width, rect.height, cl->format.bitsPerPixel);
  wxAlphaPixelData region_data(region);
  wxAlphaPixelData::Iterator region_it(region_data);
#ifdef __WIN32__
  // windows DIBs store data from bottom to top :-(
  // so move iterator to the last row
  region_it.OffsetY(region_data, rect.height-1);
#endif



  // get an wxAlphaPixelData from the framebuffer representing the subregion we want
#ifdef __WIN32__
  // windows DIBs store data from bottom to top, so sub-bitmap access like here
  // is mirrored as well...
  wxRect mirrorrect = rect;
  mirrorrect.y = framebuffer->GetHeight() - rect.y - rect.height;
  wxAlphaPixelData fbsub_data(*framebuffer, mirrorrect);
#else
  wxAlphaPixelData fbsub_data(*framebuffer, rect);
#endif
  if ( ! fbsub_data )
    {
      wxLogDebug(wxT("Failed to gain raw access to fb_sub data!"));
      return region;
    }
  wxAlphaPixelData::Iterator fbsub_it(fbsub_data);


  for( int y = 0; y < rect.height; ++y )
    {
      wxAlphaPixelData::Iterator region_it_rowStart = region_it;
      wxAlphaPixelData::Iterator fbsub_it_rowStart = fbsub_it;

      for( int x = 0; x < rect.width; ++x, ++region_it, ++fbsub_it )
	{
#ifdef __WIN32__
	  // windows stores DIB data in BGRA, not RGBA.
	  region_it.Red() = fbsub_it.Blue();
	  region_it.Blue() = fbsub_it.Red();
#else
	  region_it.Red() = fbsub_it.Red();
	  region_it.Blue() = fbsub_it.Blue();
#endif	  
	  region_it.Green() = fbsub_it.Green();	  
	  region_it.Alpha() = 255; // 255 is opaque, libvncclient always sets this byte to 0
	}
      
      // this goes downwards
      region_it = region_it_rowStart;
#ifdef __WIN32__
      region_it.OffsetY(region_data, -1);
#else
      region_it.OffsetY(region_data, 1);
#endif
     
      fbsub_it = fbsub_it_rowStart;
      fbsub_it.OffsetY(fbsub_data, 1);
    }

  return region;
 
}



int VNCConn::getFrameBufferWidth() const
{
  if(cl)
    return cl->width;
  else 
    return 0;
}


int VNCConn::getFrameBufferHeight() const
{
  if(cl)
    return cl->height;
  else 
    return 0;
}



wxString VNCConn::getDesktopName() const
{
  if(cl)
    return wxString(cl->desktopName, wxConvUTF8);
  else
    return wxEmptyString;
}


wxString VNCConn::getServerName() const
{
  if(cl)
    {
      wxIPV4address a;
      a.Hostname(wxString(cl->serverHost, wxConvUTF8));
      return a.Hostname();
    }
  else
    return wxEmptyString;
}

wxString VNCConn::getServerAddr() const
{
  if(cl)
    {
      wxIPV4address a;
      a.Hostname(wxString(cl->serverHost, wxConvUTF8));
      return a.IPAddress();
    }
  else
    return wxEmptyString;
}

wxString VNCConn::getServerPort() const
{
  if(cl)
    return wxString() << cl->serverPort;
  else
    return wxEmptyString;
}
