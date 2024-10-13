// -*- C++ -*- 
/* 
   VNCConn.h: VNC connection class API definition.

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

#ifndef VNCCONN_H
#define VNCCONN_H

#include <deque>
#include <wx/event.h>
#include <wx/string.h>
#include <wx/arrstr.h>
#include <wx/bitmap.h>
#include <wx/rawbmp.h>
#include <wx/timer.h>
#include <wx/thread.h>
#include <wx/secretstore.h>
#include <wx/msgqueue.h>
#include "rfb/rfbclient.h"



/*
  custom events
*/
/// Sent when Listen() completes with success(listening, m_commandInt==0) or failure (m_commandInt!=0)
DECLARE_EVENT_TYPE(VNCConnListenNOTIFY, -1)
/// Sent when Init() completes with success(connection established, m_commandInt==0) or failure (m_commandInt!=0)
DECLARE_EVENT_TYPE(VNCConnInitNOTIFY, -1)
/// Sent when this VNCConn wants a password. This blocks the VNCConn's internal worker thread until SetPassword() is called!
wxDECLARE_EVENT(VNCConnGetPasswordNOTIFY, wxCommandEvent);
/// Sent when this VNCConn wants a username and password. This blocks the VNCConn's internal worker thread until SetPassword() is called!
wxDECLARE_EVENT(VNCConnGetCredentialsNOTIFY, wxCommandEvent);
// sent when an incoming connection is available
DECLARE_EVENT_TYPE(VNCConnIncomingConnectionNOTIFY, -1)
// sent on disconnect
DECLARE_EVENT_TYPE(VNCConnDisconnectNOTIFY, -1)
// sent on framebuffer resize, get new size via getFrameBufferWidth/Height()
DECLARE_EVENT_TYPE(VNCConnFBResizeNOTIFY, -1)
// sent when new cuttext is available
DECLARE_EVENT_TYPE(VNCConnCuttextNOTIFY, -1) 
// sent when bell message received
DECLARE_EVENT_TYPE(VNCConnBellNOTIFY, -1) 
// sent framebuffer update, event's rect is set to region
DECLARE_EVENT_TYPE(VNCConnUpdateNOTIFY, -1)
// sent when status changes from/to uni/-multicast. 
// get current state via isMulticast()
DECLARE_EVENT_TYPE(VNCConnUniMultiChangedNOTIFY, -1) 
// sent when userinput replay finished
DECLARE_EVENT_TYPE(VNCConnReplayFinishedNOTIFY, -1) 


/**
   To make a listening connection, call Listen().
   You'll be informed by a VNCConnListenNOTIFY event about the outcome.
   You'll get a VNCConnIncomingConnectionNOTIFY event when a connection is made from the outside;
   in this case, call Init() with empty host and port to finalise the connection to the remote.

   To make an outgoing connection, call Init().
   You'll be informed by a VNCConnInitNOTIFY event about the outcome.
   You'll get a VNCConnDisconnectNOTIFY when the connection is unexpectedly terminated.

   To shut down a connection, call Shutdown().
 */
class VNCConn: public wxEvtHandler, public wxThreadHelper
{
public:
  VNCConn(void *parent);
  ~VNCConn(); 

  void Listen(int port);
  void Init(const wxString& host, const wxString& username,
#if wxUSE_SECRETSTORE
	    const wxSecretValue& password,
#endif
	    const wxString& encodings, int compresslevel = 1, int quality = 5, bool multicast = true, int multicastSocketRecvBuf = 5120, int multicastRecvBuf = 5120);
  void Shutdown();


  /*
    This is for usage on high latency links: Keep asking for framebuffer 
    updates every 'interval' ms instead of asking after every received 
    server message.
    0 to disable. default: disabled
  */
  void setFastRequest(size_t interval);

  /*
    enables marking the DSCP/Traffic Class of outgoing IP/IPv6 packets
  */
  bool setDSCP(uint8_t dscp);

  // get kind of VNCConn
  bool isReverse() const { return cl ? cl->listenSpecified : false; };
  bool isMulticast() const;

  // send events
  void sendPointerEvent(wxMouseEvent &event);
  bool sendKeyEvent(wxKeyEvent &event, bool down, bool isChar);
  
  // toggle statistics, default is off
  void doStats(bool yesno);
  // this clears internal statistics
  void resetStats();
  //  get stats, format is described in first line
  const wxArrayString& getStats() const { const wxArrayString& ref = statistics; return ref; };

  /*
    replay/record user interaction
  */
  bool replayUserInputStart(wxArrayString src, bool loop); // copies in src and plays it
  bool replayUserInputStop();
  bool recordUserInputStart();
  bool recordUserInputStop(wxArrayString& dst); // if ok, copies recorded input to dst
  bool isReplaying() { return replaying; };
  bool isRecording() { return recording; };


  // cuttext
  const wxString& getCuttext() const { const wxString& ref = cuttext; return ref; };
  void setCuttext(const wxString& text) { wxCriticalSectionLocker lock(mutex_cuttext); cuttext = text; };

  // returns a wxBitmap (this uses COW, so is okay)
  wxBitmap getFrameBufferRegion(const wxRect& region) const;
  // writes requested region directly into dst bitmap which must have the same dimensions as the framebuffer
  bool getFrameBufferRegion(const wxRect& rect, wxBitmap& dst) const;
  int getFrameBufferWidth() const;
  int getFrameBufferHeight() const;
  int getFrameBufferDepth() const;

  wxString getDesktopName() const;
  wxString getServerHost() const;
  wxString getServerPort() const;
  wxString getListenPort() const;

  const wxString& getUserName() const;
  void setUserName(const wxString& username);
#if wxUSE_SECRETSTORE
  const wxSecretValue& getPassword() const;
  void setPassword(const wxSecretValue& password);
#else
  const wxString& getPassword() const;
  void setPassword(const wxString& password);
#endif
  const bool getRequireAuth() const;


  // get current multicast receive buf state
  int getMCBufSize() const { if(cl) return cl->multicastRcvBufSize; else return 0; };
  int getMCBufFill() const { if(cl) return cl->multicastRcvBufLen; else return 0; };
  // returns average (over last few seconds) NACKed ratio or -1 if there was nothing to be measured
  double getMCNACKedRatio();
  // returns average (over last few seconds) loss ratio or -1 if there was nothing to be measured
  double getMCLossRatio();
 
  // get error string
  const wxString& getErr() const { const wxString& ref = err; return ref; };
  // get global log string
  static const wxArrayString& getLog() { const wxArrayString& ref = log; return ref; };
  static void clearLog();
  static void doLogfile(bool yesno) { do_logfile = yesno; };

  static int getMaxSocketRecvBufSize();

    /**
       Returns the VNCConn handling the given rfbClient.
     */
    static VNCConn* getVNCConnFromRfbClient(rfbClient *cl);

protected:
  // thread execution starts here
  virtual wxThread::ExitCode Entry();

  DECLARE_EVENT_TABLE();

private:
  void *parent;

  rfbClient* cl;
  bool setupClient();

  wxRect updated_rect;

  int multicastStatus;
  std::deque<double> multicastNACKedRatios;
  std::deque<double> multicastLossRatios;
#define MULTICAST_RATIO_SAMPLES 10 // we are averaging over this many seconds
  wxCriticalSection mutex_multicastratio; // the fifos above are read by both the VNC and the GUI thread
  wxStopWatch  multicastratio_stopwatch;

  
#ifdef LIBVNCSERVER_WITH_CLIENT_TLS
  static bool TLS_threading_initialized;
#endif

  // this counts the ms since Init()
  wxStopWatch conn_stopwatch;

  // fastrequest stuff
  size_t fastrequest_interval;
  wxStopWatch fastrequest_stopwatch;

  // this contains cuttext we received or should send
  wxString cuttext;
  wxCriticalSection mutex_cuttext;

  // credentials
  wxString username;
#if wxUSE_SECRETSTORE
  wxSecretValue password;
#else
  wxString password;
#endif
  bool require_auth;
  wxMutex mutex_auth;
  wxCondition condition_auth;

  // statistics
  bool do_stats;
  wxTimer stats_timer; // a timer that samples statistics every second
  void on_stats_timer(wxTimerEvent& event);
  wxCriticalSection mutex_stats;
  wxArrayString statistics;
  // counts received (probably compressed) bytes of updates
  int upd_bytes;
  // counts uncompressed bytes of updates
  int upd_bytes_inflated;
  // counts updates 
  int upd_count; 
  // check latency by isueing an xvp request with some unsupported version
#define LATENCY_TEST_XVP_VER 42
  bool latency_test_xvpmsg_sent;
  // when xvp is not available, check latency by requesting a certain test rect as non-incremental
#define LATENCY_TEST_RECT 0,0,1,1
  bool latency_test_rect_sent;
  bool latency_test_trigger;
  wxStopWatch latency_stopwatch;
  int latency;

  // record/replay stuff
  wxArrayString userinput;
  size_t userinput_pos;
  wxStopWatch recordreplay_stopwatch;
  bool replaying;
  bool replay_loop;
  bool recording;
  wxCriticalSection mutex_recordreplay;

  // per-connection error string
  wxString err;
  
  // global libvcnclient log stuff
  // there's no per-connection log since we cannot find out which client
  // called the logger function :-(
  static wxArrayString log;
  static wxCriticalSection mutex_log;
  static bool do_logfile;

  // utility functions
  static void parseHostString(const char *server, int defaultport, char **host, int *port);
 
  // messagequeues for posting events to the worker thread
  typedef wxMouseEvent pointerEvent;
  struct keyEvent
  {
    rfbKeySym keysym;
    bool down; 
  };
  wxMessageQueue<pointerEvent> pointer_event_q;
  wxMessageQueue<keyEvent> key_event_q;

  bool thread_listenmode;
  bool thread_send_pointer_event(pointerEvent &event);
  bool thread_send_key_event(keyEvent &event);
  bool thread_send_latency_probe();


  // event dispatchers
  void thread_post_listen_notify(int error);
  void thread_post_init_notify(int error);
  void thread_post_getpasswd_notify();
  void thread_post_getcreds_notify(bool withUserPrompt);
  void thread_post_incomingconnection_notify();
  void thread_post_disconnect_notify();
  void thread_post_update_notify(int x, int y, int w, int h);
  void thread_post_fbresize_notify();
  void thread_post_cuttext_notify();
  void thread_post_bell_notify();
  void thread_post_unimultichanged_notify();
  void thread_post_replayfinished_notify();

  // libvncclient callbacks
  static rfbBool thread_alloc_framebuffer(rfbClient* client);
  static void thread_got_update(rfbClient* cl,int x,int y,int w,int h);
  static void thread_update_finished(rfbClient* client);
  static void thread_kbd_leds(rfbClient* cl, int value, int pad);
  static void thread_textchat(rfbClient* cl, int value, char *text);
  static void thread_got_cuttext(rfbClient *cl, const char *text, int len);
  static void thread_bell(rfbClient *cl);
  static void thread_handle_xvp(rfbClient *cl, uint8_t ver, uint8_t code);
  static void thread_logger(const char *format, ...);
  static char* thread_getpasswd(rfbClient* client);
  static rfbCredential* thread_getcreds(rfbClient* client, int type);
};




// the custom VNCConnUpdateNotifyEvent
class VNCConnUpdateNotifyEvent: public wxCommandEvent
{
public:
  wxRect rect;

  VNCConnUpdateNotifyEvent(wxEventType commandType = VNCConnUpdateNOTIFY, int id = 0 )
    :  wxCommandEvent(commandType, id) { }
 
  // You *must* copy here the data to be transported
  VNCConnUpdateNotifyEvent( const VNCConnUpdateNotifyEvent &event )
    :  wxCommandEvent(event) { this->rect = event.rect; }
 
  // Required for sending with wxPostEvent()
  wxEvent* Clone() const { return new VNCConnUpdateNotifyEvent(*this); }
 };
 

// This #define simplifies the one below, and makes the syntax less
// ugly if you want to use Connect() instead of an event table.
typedef void (wxEvtHandler::*VNCConnUpdateNotifyEventFunction)(VNCConnUpdateNotifyEvent &);
#define VNCConnUpdateNotifyEventHandler(func)				\
  (wxObjectEventFunction)(wxEventFunction)(wxCommandEventFunction)	\
  wxStaticCastEvent(VNCConnUpdateNotifyEventFunction, &func)                    
 
// Define the event table entry. Yes, it really *does* end in a comma.
#define EVT_VNCCONNUPDATENOTIFY(id, fn)					\
  DECLARE_EVENT_TABLE_ENTRY(VNCConnUpdateNOTIFY, id, wxID_ANY,		\
			    (wxObjectEventFunction)(wxEventFunction)	\
			    (wxCommandEventFunction)			\
			    wxStaticCastEvent(VNCConnUpdateNotifyEventFunction, &fn ), (wxObject*) NULL ),
 



#endif
