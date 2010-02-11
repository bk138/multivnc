/* 
   VNCConn.cpp: VNC connection class implemtation.

   This file is part of MultiVNC, a multicast-enabled crossplatform 
   VNC viewer.
 
   Copyright (C) 2009, 2010 Christian Beier <dontmind@freeshell.org>
 
   MultiVNC is free software; you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation; either version 2 of the License, or 
   (at your option) any later version. 
 
   MultiVNC is distributed in the hope that it will be useful, 
   but WITHOUT ANY WARRANTY; without even the implied warranty of 
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
   GNU General Public License for more details. 
 
   You should have received a copy of the GNU General Public License 
   along with this program; if not, write to the Free Software 
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. 
*/


#include <cstdarg>
#include <csignal>
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/thread.h>
#include <wx/socket.h>
#include "msgqueue.h"
#include "VNCConn.h"


// use some global address
#define VNCCONN_OBJ_ID (void*)VNCConn::got_update

// logfile name
#define LOGFILE _T("MultiVNC.log")


// define our new notify events!
DEFINE_EVENT_TYPE(VNCConnIncomingConnectionNOTIFY)
DEFINE_EVENT_TYPE(VNCConnDisconnectNOTIFY)
DEFINE_EVENT_TYPE(VNCConnUpdateNOTIFY)
DEFINE_EVENT_TYPE(VNCConnFBResizeNOTIFY)
DEFINE_EVENT_TYPE(VNCConnCuttextNOTIFY) 
DEFINE_EVENT_TYPE(VNCConnUniMultiChangedNOTIFY);

// these are passed to the worker thread
typedef wxMouseEvent pointerEvent;
struct keyEvent
{
  rfbKeySym keysym;
  bool down; 
};


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
  bool listenMode;

  bool sendPointerEvent(pointerEvent &event);
  bool sendKeyEvent(keyEvent &event);
 
public:
  wxMessageQueue<pointerEvent> pointer_event_q;
  wxMessageQueue<keyEvent> key_event_q;

  VNCThread(VNCConn *parent, bool listen);
  
  // thread execution starts here
  virtual wxThread::ExitCode Entry();
  
  // called when the thread exits - whether it terminates normally or is
  // stopped with Delete() (but not when it is Kill()ed!)
  virtual void OnExit();
};





VNCThread::VNCThread(VNCConn *parent, bool listen)
  : wxThread()
{
  p = parent;
  listenMode = listen;
}




bool VNCThread::sendPointerEvent(pointerEvent &event)
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

  if(event.Entering() && ! p->cuttext.IsEmpty())
    {
      wxCriticalSectionLocker lock(p->mutex_cuttext); // since cuttext can be set from the main thread
      // if encoding fails, a NULL pointer is returned!
      if(p->cuttext.mb_str(wxCSConv(wxT("iso-8859-1"))))
	{
	  wxLogDebug(wxT("VNCConn %p: sending cuttext: '%s'"), p, p->cuttext.c_str());
	  char* encoded_text = strdup(p->cuttext.mb_str(wxCSConv(wxT("iso-8859-1"))));
	  SendClientCutText(p->cl, encoded_text, strlen(encoded_text));
	  free(encoded_text);	  
	}
      else
	wxLogDebug(wxT("VNCConn %p: sending cuttext FAILED, could not convert '%s' to ISO-8859-1"), p, p->cuttext.c_str());
    }

  if(p->do_stats)
    {
      wxCriticalSectionLocker lock(p->mutex_latency_stats);
      p->pointer_pos.x = event.m_x;
      p->pointer_pos.y = event.m_y;
      p->pointer_stopwatch.Start();
    }

  wxLogDebug(wxT("VNCConn %p: sending pointer event at (%d,%d), buttonmask %d"), p, event.m_x, event.m_y, buttonmask);
  return SendPointerEvent(p->cl, event.m_x, event.m_y, buttonmask);
}




bool VNCThread::sendKeyEvent(keyEvent &event)
{
  return SendKeyEvent(p->cl, event.keysym, event.down);
}




wxThread::ExitCode VNCThread::Entry()
{
  int i=0;

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

  pointerEvent pe;
  keyEvent ke = {0, 0};

  while(! TestDestroy()) 
    {
#ifdef __WXGTK__
      sigprocmask(SIG_BLOCK, &newsigs, &oldsigs);
#endif

      if(listenMode)
	i=listenForIncomingConnectionsNoFork(p->cl, 500);
      else
	{
	  // send everything that's inside the input queues
	  while(pointer_event_q.ReceiveTimeout(0, pe) != wxMSGQUEUE_TIMEOUT) // timeout == empty
	    sendPointerEvent(pe);
	  while(key_event_q.ReceiveTimeout(0, ke) != wxMSGQUEUE_TIMEOUT) // timeout == empty
	    sendKeyEvent(ke);

	  if(!rfbProcessServerMessage(p->cl, 500))
	    {
	      wxLogDebug(wxT("VNCConn %p: vncthread rfbProcessServerMessage() failed"), p);
	      p->post_disconnect_notify();
	      break;
	    }

	  int now = p->isMulticast();
	  if(now != p->multicastStatus)
	    {
	      p->multicastStatus = now;
	      p->post_unimultichanged_notify();
	    }
	}

#ifdef __WXGTK__
      sigprocmask(SIG_SETMASK, &oldsigs, NULL);
#endif
      
      if(listenMode)
	{
	  if(i<0)
	    {
	      wxLogDebug(wxT("VNCConn %p: vncthread listen() failed"), p);
	      p->post_disconnect_notify();
	      return 0;
	    }
	  if(i)
	    {
	      p->post_incomingconnection_notify();
	      return 0;
	    }
	}
    }
  return 0;
}



void VNCThread::OnExit()
{
  // cause wxThreads delete themselves after completion
  p->vncthread = 0;
  wxLogDebug(wxT("VNCConn %p: %s vncthread exiting"), p, listenMode ? wxT("listening") : wxT(""));
}






/********************************************

  VNCConn class

********************************************/

BEGIN_EVENT_TABLE(VNCConn, wxEvtHandler)
    EVT_TIMER (wxID_ANY, VNCConn::on_updatescount_timer)
END_EVENT_TABLE();


#ifdef LIBVNCSERVER_WITH_CLIENT_TLS
bool VNCConn::TLS_threading_initialized;
extern "C" 
{
#include <gcrypt.h>
#include <errno.h>
  GCRY_THREAD_OPTION_PTHREAD_IMPL;
}
#endif


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
  multicastStatus = 0;

  rfbClientLog = rfbClientErr = logger;

#ifdef LIBVNCSERVER_WITH_CLIENT_TLS
  /* we're using threads in here, tell libgcrypt before TLS 
     gets initialized by libvncclient! */
  if(! TLS_threading_initialized)
    {
      wxLogDebug(wxT("Initialized libgcrypt threading."));
      gcry_control (GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
      gcry_check_version (NULL);
      gcry_control (GCRYCTL_DISABLE_SECMEM, 0);
      gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0);
      TLS_threading_initialized = true;
    }
#endif

  // statistics stuff
  do_stats = false;
  updates_count = 0;
  updates_count_timer.SetOwner(this);
}




VNCConn::~VNCConn()
{
  Shutdown();
  Cleanup();
  wxLogDebug(wxT("VNCConn %p: I'm dead!"), this);
}



/*
  private members
*/

void VNCConn::post_incomingconnection_notify() 
{
  wxLogDebug(wxT("VNCConn %p: post_incomingconnection_notify()"), this);

  // new NOTIFY event, we got no window id
  wxCommandEvent event(VNCConnIncomingConnectionNOTIFY, wxID_ANY);
  event.SetEventObject(this); // set sender

  // Send it
  wxPostEvent((wxEvtHandler*)parent, event);
}


void VNCConn::post_disconnect_notify() 
{
  wxLogDebug(wxT("VNCConn %p: post_disconnect_notify()"), this);

  // new NOTIFY event, we got no window id
  wxCommandEvent event(VNCConnDisconnectNOTIFY, wxID_ANY);
  event.SetEventObject(this); // set sender

  // Send it
  wxPostEvent((wxEvtHandler*)parent, event);
}


void VNCConn::post_update_notify(int x, int y, int w, int h)
{
  VNCConnUpdateNotifyEvent event(VNCConnUpdateNOTIFY, wxID_ANY);
  event.SetEventObject(this); // set sender

  // set info about what was updated
  event.rect = wxRect(x, y, w, h);
  wxLogDebug(wxT("VNCConn %p: post_update_notify(%i,%i,%i,%i)"), this,
	     event.rect.x,
	     event.rect.y,
	     event.rect.width,
	     event.rect.height);
  
  // Send it
  wxPostEvent((wxEvtHandler*)parent, event);

  if(do_stats)
    {
      // raw byte updates/second
      updates_count += w*h*BYTESPERPIXEL;
  
      // pointer latency
      // well, this is not neccessarily correct, but wtf
      if(event.rect.Contains(pointer_pos))
	{
	  pointer_stopwatch.Pause();
	  wxCriticalSectionLocker lock(mutex_latency_stats);
	  latencies.Add((wxString() << (int)conn_stopwatch.Time()) + 
			wxT(", ") + 
			(wxString() << (int)pointer_stopwatch.Time()));

	  wxLogDebug(wxT("VNCConn %p: got update at pointer position, latency %ims"), this, pointer_stopwatch.Time());
	}
    }
}


void VNCConn::post_fbresize_notify() 
{
  wxLogDebug(wxT("VNCConn %p: post_fbresize_notify() (%i, %i)"), 
	     this,
	     getFrameBufferWidth(),
	     getFrameBufferHeight());

  // new NOTIFY event, we got no window id
  wxCommandEvent event(VNCConnFBResizeNOTIFY, wxID_ANY);
  event.SetEventObject(this); // set sender

  // Send it
  wxPostEvent((wxEvtHandler*)parent, event);
}



void VNCConn::post_cuttext_notify()
{
  // new NOTIFY event, we got no window id
  wxCommandEvent event(VNCConnCuttextNOTIFY, wxID_ANY);
  event.SetEventObject(this); // set sender

  // Send it
  wxPostEvent((wxEvtHandler*)parent, event);
}



void VNCConn::post_unimultichanged_notify()
{
  wxCommandEvent event(VNCConnUniMultiChangedNOTIFY, wxID_ANY);
  event.SetEventObject(this); // set sender
  
  // Send it
  wxPostEvent((wxEvtHandler*)parent, event);
}



void VNCConn::on_updatescount_timer(wxTimerEvent& event)
{
  if(do_stats)
    {
      wxCriticalSectionLocker lock(mutex_latency_stats);
      updates.Add((wxString() << (int)conn_stopwatch.Time())
		  + wxT(", ") +
		  (wxString() << updates_count));
      updates_count = 0;

      if(isMulticast())
	{
	  double lossrate = cl->multicastLost/(double)(cl->multicastRcvd+cl->multicastLost);
	  mc_lossratios.Add((wxString() << (int)conn_stopwatch.Time())
			    + wxT(", ") +
			    wxString::Format(wxT("%.4f"), lossrate));
	}
    }
}



rfbBool VNCConn::alloc_framebuffer(rfbClient* client)
{
  // get VNCConn object belonging to this client
  VNCConn* conn = (VNCConn*) rfbClientGetClientData(client, VNCCONN_OBJ_ID); 

  wxLogDebug(wxT("VNCConn %p: alloc'ing framebuffer w:%i, h:%i"), conn, client->width, client->height);

  // assert 32bpp, as requested with GetClient() in Init()
  if(client->format.bitsPerPixel != 32)
    {
      conn->err.Printf(_("Failure setting up framebuffer: wrong BPP!"));
      return false;
    }

  // ensure that we get the whole framebuffer in case of a resize!
  client->updateRect.x = client->updateRect.y = 0;
  client->updateRect.w = client->width; client->updateRect.h = client->height;

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
 
  // notify our parent
  conn->post_fbresize_notify();
 
  return client->frameBuffer ? TRUE : FALSE;
}





void VNCConn::got_update(rfbClient* client,int x,int y,int w,int h)
{
  VNCConn* conn = (VNCConn*) rfbClientGetClientData(client, VNCCONN_OBJ_ID); 
 
  conn->post_update_notify(x, y, w, h);
}





void VNCConn::kbd_leds(rfbClient* cl, int value, int pad)
{
  VNCConn* conn = (VNCConn*) rfbClientGetClientData(cl, VNCCONN_OBJ_ID); 
  wxLogDebug(wxT("VNCConn %p: Led State= 0x%02X\n"), conn, value);
}


void VNCConn::textchat(rfbClient* cl, int value, char *text)
{
  VNCConn* conn = (VNCConn*) rfbClientGetClientData(cl, VNCCONN_OBJ_ID); 
  switch(value)
    {
    case rfbTextChatOpen:
      wxLogDebug(wxT("VNCConn %p: got textchat open\n"), conn);
      break;
    case rfbTextChatClose:
      wxLogDebug(wxT("VNCConn %p: got textchat close\n"), conn);
      break;
    case rfbTextChatFinished:
      wxLogDebug(wxT("VNCConn %p: got textchat finish\n"), conn);
      break;
    default:
      wxLogDebug(wxT("VNCConn %p: got textchat text: '%s'\n"), conn, text);
    }
}


void VNCConn::got_cuttext(rfbClient *cl, const char *text, int len)
{
  VNCConn* conn = (VNCConn*) rfbClientGetClientData(cl, VNCCONN_OBJ_ID);

  wxLogDebug(wxT("VNCConn %p: got cuttext: '%s'"), conn, wxString(text, wxCSConv(wxT("iso-8859-1"))).c_str());

  wxCriticalSectionLocker lock(conn->mutex_cuttext); // since cuttext can also be set from the main thread
  conn->cuttext = wxString(text, wxCSConv(wxT("iso-8859-1")));
  conn->post_cuttext_notify();
}



// there's no per-connection log since we cannot find out which client
// called the logger function :-(
wxArrayString VNCConn::log;
wxCriticalSection VNCConn::mutex_log;
bool VNCConn::do_logfile;

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

  time(&log_clock);
  wxStrftime(timebuf, WXSIZEOF(timebuf), _T("%d/%m/%Y %X "), localtime(&log_clock));

  // global log string array
  va_start(args, format);
  char msg[1024];
  vsnprintf(msg, 1024, format, args);
  log.Add( wxString(timebuf) + wxString(msg, wxConvUTF8));
  va_end(args);

  // global log file
  if(do_logfile)
    {
      // delete logfile on program startup
      static bool firstrun = 1;
      if(firstrun)
	{
	  remove(logfile_str.char_str());
	  firstrun = 0;
	}
      
      logfile=fopen(logfile_str.char_str(),"a");    
      
      va_start(args, format);
      fprintf(logfile, wxString(timebuf).mb_str());
      vfprintf(logfile, format, args);
      va_end(args);

      fclose(logfile);
    }
 
  // and stderr
  va_start(args, format);
  fprintf(stderr, wxString(timebuf).mb_str());
  vfprintf(stderr, format, args);
  va_end(args);
}






/*
  public members
*/

bool VNCConn::Setup(char* (*getpasswdfunc)(rfbClient*))
{
  wxLogDebug(wxT("VNCConn %p: Setup()"), this);

  if(cl) // already set up
    {
      wxLogDebug(wxT("VNCConn %p: Setup already done. Call Cleanup() first!"), this);
      return false;
    }

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
  cl->GotXCutText = got_cuttext;

  cl->canHandleNewFBSize = TRUE;
  
  return true;
}


void VNCConn::Cleanup()
{
  wxLogDebug(wxT( "VNCConn %p: Cleanup()"), this);

  if(cl)
    {
      wxLogDebug(wxT( "VNCConn %p: Cleanup() before client cleanup"), this);
      rfbClientCleanup(cl);
      cl = 0;
      wxLogDebug(wxT( "VNCConn %p: Cleanup() after client cleanup"), this);
    }
}


bool VNCConn::Listen(int port)
{
  wxLogDebug(wxT("VNCConn %p: Listen() port %d"), this, port);

  cl->listenPort = port;
  
  VNCThread *tp = new VNCThread(this, true);
  // save it for later on
  vncthread = tp;
  
  if( tp->Create() != wxTHREAD_NO_ERROR )
    {
      err.Printf(_("Could not create VNC listener thread!"));
      Shutdown();
      return false;
    }

  if( tp->Run() != wxTHREAD_NO_ERROR )
    {
      err.Printf(_("Could not start VNC listener thread!")); 
      Shutdown();
      return false;
    }

  return true;
}


bool VNCConn::Init(const wxString& host, int compresslevel, int quality, bool multicast, int multicast_recvbuf)
{
  wxLogDebug(wxT("VNCConn %p: Init()"), this);

  if(fb_data || framebuffer || vncthread)
    {
      wxLogDebug(wxT("VNCConn %p: Init() already done. Call Shutdown() first!"), this);
      return false;
    }

  // reset stats before doing new connection
  resetStats();

  int argc = 6;
  char* argv[argc];
  argv[0] = strdup("VNCConn");
  argv[1] = strdup(host.mb_str());
  argv[2] = strdup("-compress");
  argv[3] = strdup((wxString() << compresslevel).mb_str());
  argv[4] = strdup("-quality");
  argv[5] = strdup((wxString() << quality).mb_str());
  if(multicast)
    {
      cl->canHandleMulticastVNC = TRUE;
      cl->multicastRcvBufSize = multicast_recvbuf*1024;
    }
  else
    cl->canHandleMulticastVNC = FALSE;

  if(! rfbInitClient(cl, &argc, argv))
    {
      cl = 0; //  rfbInitClient() calls rfbClientCleanup() on failure, but this does not zero the ptr
      err.Printf(_("Failure connecting to server at %s!"),  host.c_str());
      wxLogDebug(wxT("VNCConn %p: Init() failed. Cleanup by library."), this);
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
  VNCThread *tp = new VNCThread(this, false);
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

  conn_stopwatch.Start();

  return true;
}




void VNCConn::Shutdown()
{
  wxLogDebug(wxT("VNCConn %p: Shutdown()"), this);

  conn_stopwatch.Pause();

  if(vncthread)
    {
      wxLogDebug(wxT( "VNCConn %p: Shutdown() before vncthread delete"), this);
      ((VNCThread*)vncthread)->Delete();
      // wait for deletion to finish
      while(vncthread)
	wxMilliSleep(100);
      
      wxLogDebug(wxT("VNCConn %p: Shutdown() after vncthread delete"), this);
    }
  
  if(cl)
    {
      wxLogDebug(wxT( "VNCConn %p: Shutdown() closing connection"), this);
#ifdef __WIN32__
      closesocket(cl->sock);
#else
      close(cl->sock);
#endif
      // in case we called listen, canceled that, and now want to connect to some
      // host via Init()
      cl->listenSpecified = FALSE;
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



// this simply posts the mouse event into the worker thread's input queue
void VNCConn::sendPointerEvent(wxMouseEvent &event)
{
  
  ((VNCThread*)vncthread)->pointer_event_q.Post(event);
}


// because of the possible wxKeyEvent.Skip(), this posts the found keysym + down
bool VNCConn::sendKeyEvent(wxKeyEvent &event, bool down, bool isChar)
{
  keyEvent kev = {0,0};

  if(isChar)
    {
      wxLogDebug(wxT("VNCConn %p: got CHAR key event"), this);
      wxLogDebug(wxT("VNCConn %p:     wxkeycode:      %d"), this, event.GetKeyCode());
      wxLogDebug(wxT("VNCConn %p:     wxkeycode char: %c"), this, event.GetKeyCode());
      wxLogDebug(wxT("VNCConn %p:     unicode:        %d"), this, event.GetUnicodeKey());
      wxLogDebug(wxT("VNCConn %p:     unicode char:   %c"), this, event.GetUnicodeKey()); 

      // get tranlated char
      kev.keysym = event.GetKeyCode();
      // if we got ne keycode, try unicodekey
      if(kev.keysym==0) 
	kev.keysym = event.GetUnicodeKey();

      // if wxwidgets translates a key combination into a
      // value below 32, revert this here.
      // we dont't send ASCII 0x03, but ctrl and then a 'c'!
      if(kev.keysym <= 32)
	{
	  kev.keysym += 96;
	  wxLogDebug(wxT("VNCConn %p: translating key to: %d"), this, kev.keysym);
	}

      wxLogDebug(wxT("VNCConn %p: sending rfbkeysym: 0x%.3x down"), this, kev.keysym);
      wxLogDebug(wxT("VNCConn %p: sending rfbkeysym: 0x%.3x  up"), this, kev.keysym);
      
      // down, then up
      kev.down = true;
      ((VNCThread*)vncthread)->key_event_q.Post(kev);
      kev.down = false;
      ((VNCThread*)vncthread)->key_event_q.Post(kev);
      return true;
    }
  else
    {
      // lookup keysym  
      switch(event.GetKeyCode()) 
	{
	case WXK_BACK: kev.keysym = XK_BackSpace; break;
	case WXK_TAB: kev.keysym = XK_Tab; break;
	case WXK_CLEAR: kev.keysym = XK_Clear; break;
	case WXK_RETURN: kev.keysym = XK_Return; break;
	case WXK_PAUSE: kev.keysym = XK_Pause; break;
	case WXK_ESCAPE: kev.keysym = XK_Escape; break;
	case WXK_SPACE: kev.keysym = XK_space; break;
	case WXK_DELETE: kev.keysym = XK_Delete; break;
	case WXK_NUMPAD0: kev.keysym = XK_KP_0; break;
	case WXK_NUMPAD1: kev.keysym = XK_KP_1; break;
	case WXK_NUMPAD2: kev.keysym = XK_KP_2; break;
	case WXK_NUMPAD3: kev.keysym = XK_KP_3; break;
	case WXK_NUMPAD4: kev.keysym = XK_KP_4; break;
	case WXK_NUMPAD5: kev.keysym = XK_KP_5; break;
	case WXK_NUMPAD6: kev.keysym = XK_KP_6; break;
	case WXK_NUMPAD7: kev.keysym = XK_KP_7; break;
	case WXK_NUMPAD8: kev.keysym = XK_KP_8; break;
	case WXK_NUMPAD9: kev.keysym = XK_KP_9; break;
	case WXK_NUMPAD_DECIMAL: kev.keysym = XK_KP_Decimal; break;
	case WXK_NUMPAD_DIVIDE: kev.keysym = XK_KP_Divide; break;
	case WXK_NUMPAD_MULTIPLY: kev.keysym = XK_KP_Multiply; break;
	case WXK_NUMPAD_SUBTRACT: kev.keysym = XK_KP_Subtract; break;
	case WXK_NUMPAD_ADD: kev.keysym = XK_KP_Add; break;
	case WXK_NUMPAD_ENTER: kev.keysym = XK_KP_Enter; break;
	case WXK_NUMPAD_EQUAL: kev.keysym = XK_KP_Equal; break;
	case WXK_UP: kev.keysym = XK_Up; break;
	case WXK_DOWN: kev.keysym = XK_Down; break;
	case WXK_RIGHT: kev.keysym = XK_Right; break;
	case WXK_LEFT: kev.keysym = XK_Left; break;
	case WXK_INSERT: kev.keysym = XK_Insert; break;
	case WXK_HOME: kev.keysym = XK_Home; break;
	case WXK_END: kev.keysym = XK_End; break;
	case WXK_PAGEUP: kev.keysym = XK_Page_Up; break;
	case WXK_PAGEDOWN: kev.keysym = XK_Page_Down; break;
	case WXK_F1: kev.keysym = XK_F1; break;
	case WXK_F2: kev.keysym = XK_F2; break;
	case WXK_F3: kev.keysym = XK_F3; break;
	case WXK_F4: kev.keysym = XK_F4; break;
	case WXK_F5: kev.keysym = XK_F5; break;
	case WXK_F6: kev.keysym = XK_F6; break;
	case WXK_F7: kev.keysym = XK_F7; break;
	case WXK_F8: kev.keysym = XK_F8; break;
	case WXK_F9: kev.keysym = XK_F9; break;
	case WXK_F10: kev.keysym = XK_F10; break;
	case WXK_F11: kev.keysym = XK_F11; break;
	case WXK_F12: kev.keysym = XK_F12; break;
	case WXK_F13: kev.keysym = XK_F13; break;
	case WXK_F14: kev.keysym = XK_F14; break;
	case WXK_F15: kev.keysym = XK_F15; break;
	case WXK_NUMLOCK: kev.keysym = XK_Num_Lock; break;
	case WXK_CAPITAL: kev.keysym = XK_Caps_Lock; break;
	case WXK_SCROLL: kev.keysym = XK_Scroll_Lock; break;
	  //case WXK_RSHIFT: kev.keysym = XK_Shift_R; break;
	case WXK_SHIFT: kev.keysym = XK_Shift_L; break;
	  //case WXK_RCTRL: kev.keysym = XK_Control_R; break;
	case WXK_CONTROL: kev.keysym = XK_Control_L; break;
	  //    case WXK_RALT: kev.keysym = XK_Alt_R; break;
	case WXK_ALT: kev.keysym = XK_Alt_L; break;
	  // case WXK_RMETA: kev.keysym = XK_Meta_R; break;
	  // case WXK_META: kev.keysym = XK_Meta_L; break;
	case WXK_WINDOWS_LEFT: kev.keysym = XK_Super_L; break;	
	case WXK_WINDOWS_RIGHT: kev.keysym = XK_Super_R; break;	
	  //case WXK_COMPOSE: kev.keysym = XK_Compose; break;
	  //case WXK_MODE: kev.keysym = XK_Mode_switch; break;
	case WXK_HELP: kev.keysym = XK_Help; break;
	case WXK_PRINT: kev.keysym = XK_Print; break;
	  //case WXK_SYSREQ: kev.keysym = XK_Sys_Req; break;
	case WXK_CANCEL: kev.keysym = XK_Break; break;
	default: break;
	}

      if(kev.keysym)
	{
	  wxLogDebug(wxT("VNCConn %p: got key %s event:"), this, down ? wxT("down") : wxT("up"));
	  wxLogDebug(wxT("VNCConn %p:     wxkeycode:      %d"), this, event.GetKeyCode());
	  wxLogDebug(wxT("VNCConn %p:     wxkeycode char: %c"), this, event.GetKeyCode());
	  wxLogDebug(wxT("VNCConn %p:     unicode:        %d"), this, event.GetUnicodeKey());
	  wxLogDebug(wxT("VNCConn %p:     unicode char:   %c"), this, event.GetUnicodeKey()); 
	  wxLogDebug(wxT("VNCConn %p: sending rfbkeysym:  0x%.3x %s"), this, kev.keysym, down ? wxT("down") : wxT("up"));

	  kev.down = down;
	  ((VNCThread*)vncthread)->key_event_q.Post(kev);
	  return true;
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





void VNCConn::doStats(bool yesno)
{
  do_stats = yesno;
  wxCriticalSectionLocker lock(mutex_latency_stats);
  if(do_stats)
    updates_count_timer.Start(1000);
  else
    updates_count_timer.Stop();
}


void VNCConn::resetStats()
{
  wxCriticalSectionLocker lock(mutex_latency_stats);
  latencies.Clear();
  updates.Clear();
  mc_lossratios.Clear();
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
      if(cl->listenSpecified)
	return wxString(wxT("listening"));
      else
	{
	  wxIPV4address a;
	  a.Hostname(wxString(cl->serverHost, wxConvUTF8));
	  return a.Hostname();
	}
    }
  else
    return wxEmptyString;
}

wxString VNCConn::getServerAddr() const
{
  if(cl)
    {
      wxIPV4address a;
      if(!cl->listenSpecified)
	a.Hostname(wxString(cl->serverHost, wxConvUTF8));
      return a.IPAddress();
    }
  else
    return wxEmptyString;
}

wxString VNCConn::getServerPort() const
{
  if(cl)
    if(cl->listenSpecified)
      return wxString() << cl->listenPort;
    else
      return wxString() << cl->serverPort;
  else
    return wxEmptyString;
}

bool VNCConn::isMulticast() const
{
  if(cl && cl->multicastSock >= 0 && !cl->multicastDisabled) 
    return true;
  else
    return false;
}


