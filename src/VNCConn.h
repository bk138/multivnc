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

#include <wx/event.h>
#include <wx/string.h>
#include <wx/arrstr.h>
#include <wx/bitmap.h>
#include <wx/rawbmp.h>
#include <wx/timer.h>
#include <wx/thread.h>
#include "msgqueue.h"
#include "rfb/rfbclient.h"



/*
  custom events
*/
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



class VNCConn: public wxEvtHandler, public wxThreadHelper
{
public:
  VNCConn(void *parent);
  ~VNCConn(); 

  /*
    to make a connection, call
    Setup(), then
    Listen() (optional), then
    Init(), then
    Shutdown, then
    Cleanup()
    
    NB: If Init() fails, you have to call Setup() again!

    The semantic counterparts are:
       Setup() <-> Cleanup()
       Init()  <-> Shutdown()
  */

  bool Setup(char* (*getpasswdfunc)(rfbClient*));
  void Cleanup();
  bool Listen(int port);
  bool Init(const wxString& host, const wxString& encodings, int compresslevel = 1, int quality = 5, 
	    bool multicast = true, int multicastSocketRecvBuf = 5120, int multicastRecvBuf = 5120);
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
  /*
    get stats, in format "timestamp, value"
  */
  // gets raw bytes updated per second
  const wxArrayString& getUpdRawByteStats() const { const wxArrayString& ref = update_rawbytes; return ref; };
  // gets number of updates per second 
  const wxArrayString& getUpdCountStats() const { const wxArrayString& ref = update_counts; return ref; };
  // gets latencies
  const wxArrayString& getLatencyStats() const { const wxArrayString& ref = latencies; return ref; };
  // gets multicast NACKed ratio per second
  const wxArrayString& getMCNACKedRatioStats() const { const wxArrayString& ref = multicast_nackedratios; return ref; };
  // gets multicast loss ratio per second
  const wxArrayString& getMCLossRatioStats() const { const wxArrayString& ref = multicast_lossratios; return ref; };
  // gets multicast receive buf fill per second
  const wxArrayString& getMCBufStats() const { const wxArrayString& ref = multicast_bufferfills; return ref; };

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

  // get current multicast receive buf state
  int getMCBufSize() const { if(cl) return cl->multicastRcvBufSize; else return 0; };
  int getMCBufFill() const { if(cl) return cl->multicastRcvBufLen; else return 0; };
 
  // get error string
  const wxString& getErr() const { const wxString& ref = err; return ref; };
  // get global log string
  static const wxArrayString& getLog() { const wxArrayString& ref = log; return ref; };
  static void clearLog();
  static void doLogfile(bool yesno) { do_logfile = yesno; };

  static int getMaxSocketRecvBufSize();

protected:
  // thread execution starts here
  virtual wxThread::ExitCode Entry();

  DECLARE_EVENT_TABLE();

private:
  void *parent;

  rfbClient* cl;

  wxRect updated_rect;

  int multicastStatus;
  double multicastNACKedRatio;
  double multicastLossRatio;
  
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

  // statistics
  bool do_stats;
  wxTimer stats_timer; // a timer that samples stattistics every second
  void on_stats_timer(wxTimerEvent& event);
  wxCriticalSection mutex_stats;
  // counts raw bytes of updates
  int upd_rawbytes;
  wxArrayString update_rawbytes;
  // counts updates 
  int upd_count; 
  wxArrayString update_counts;
  // check latency by isueing an xvp request with some unsupported version
#define LATENCY_TEST_XVP_VER 42
  bool latency_test_xvpmsg_sent;
  // when xvp is not available, check latency by requesting a certain test rect as non-incremental
#define LATENCY_TEST_RECT 0,0,1,1
  bool latency_test_rect_sent;
  bool latency_test_trigger;
  wxStopWatch latency_stopwatch;
  wxArrayString latencies;
  // mc NACKed ratios
  wxArrayString multicast_nackedratios;
  // mc loss ratios
  wxArrayString multicast_lossratios;
  // mc buffer stats
  wxArrayString multicast_bufferfills;

  // per-connection error string
  wxString err;
  
  // global libvcnclient log stuff
  // there's no per-connection log since we cannot find out which client
  // called the logger function :-(
  static wxArrayString log;
  static wxCriticalSection mutex_log;
  static bool do_logfile;

 
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
  void thread_post_incomingconnection_notify();
  void thread_post_disconnect_notify();
  void thread_post_update_notify(int x, int y, int w, int h);
  void thread_post_fbresize_notify();
  void thread_post_cuttext_notify();
  void thread_post_bell_notify();
  void thread_post_unimultichanged_notify();


  // libvncclient callbacks
  static rfbBool alloc_framebuffer(rfbClient* client);
  static void thread_got_update(rfbClient* cl,int x,int y,int w,int h);
  static void thread_update_finished(rfbClient* client);
  static void thread_kbd_leds(rfbClient* cl, int value, int pad);
  static void thread_textchat(rfbClient* cl, int value, char *text);
  static void thread_got_cuttext(rfbClient *cl, const char *text, int len);
  static void thread_bell(rfbClient *cl);
  static void thread_handle_xvp(rfbClient *cl, uint8_t ver, uint8_t code);
  static void thread_logger(const char *format, ...);
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
