// -*- C++ -*- 

#ifndef VNCCONN_H
#define VNCCONN_H

#include <wx/event.h>
#include <wx/string.h>
#include <wx/arrstr.h>
#include <wx/bitmap.h>
#include <wx/rawbmp.h>
#include <wx/timer.h>
#include "rfb/rfbclient.h"



// make available custom events
DECLARE_EVENT_TYPE(VNCConnIncomingConnectionNOTIFY, -1)
DECLARE_EVENT_TYPE(VNCConnDisconnectNOTIFY, -1)
DECLARE_EVENT_TYPE(VNCConnFBResizeNOTIFY, -1)
DECLARE_EVENT_TYPE(VNCConnCuttextNOTIFY, -1) 
DECLARE_EVENT_TYPE(VNCConnUpdateNOTIFY, -1)


class VNCConn: public wxEvtHandler
{
  friend class VNCThread;
  void *vncthread;

  void *parent;

  rfbClient* cl;

#ifdef LIBVNCSERVER_WITH_CLIENT_TLS
  static bool TLS_threading_initialized;
#endif

  // this counts the ms since Init()
  wxStopWatch conn_stopwatch;

  // complete framebuffer
  wxBitmap* framebuffer;
  wxAlphaPixelData* fb_data;

  // this contains cuttext we received or should send
  wxString cuttext;
  wxCriticalSection mutex_cuttext;

  // statistics
  bool do_stats;
  int updates_count; // counts raw bytes of updates
  wxTimer updates_count_timer; // a timer to reset updates_count periodically
  void on_updatescount_timer(wxTimerEvent& event);
  wxPoint pointer_pos;
  wxStopWatch pointer_stopwatch;
  wxCriticalSection mutex_latency_stats;
  // string arrays to store values over time
  wxArrayString updates;
  wxArrayString latencies;
  wxArrayString mc_lossratios;


  // per-connection error string
  wxString err;
  
  // global libvcnclient log stuff
  // there's no per-connection log since we cannot find out which client
  // called the logger function :-(
  static wxArrayString log;
  static wxCriticalSection mutex_log;
  static bool do_logfile;

  // event dispatchers
  void post_incomingconnection_notify();
  void post_disconnect_notify();
  void post_update_notify(int x, int y, int w, int h);
  void post_fbresize_notify();
  void post_cuttext_notify();


  //callbacks
  static rfbBool alloc_framebuffer(rfbClient* client);
  static void got_update(rfbClient* cl,int x,int y,int w,int h);
  static void kbd_leds(rfbClient* cl, int value, int pad);
  static void textchat(rfbClient* cl, int value, char *text);
  static void got_cuttext(rfbClient *cl, const char *text, int len);
  static void logger(const char *format, ...);


protected:
  DECLARE_EVENT_TABLE();


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
  bool Init(const wxString& host, int compresslevel = 1, int quality = 5, bool multicast = true);
  void Shutdown();

  bool isReverse() const { return cl ? cl->listenSpecified : false; };
  bool isMulticast() const;

  void sendPointerEvent(wxMouseEvent &event);
  bool sendKeyEvent(wxKeyEvent &event, bool down, bool isChar);
  
  // toggle statistics, default is off
  void doStats(bool yesno);
  // this clears internal statistics
  void resetStats();
  // get stats, in format "timestamp, value"
  const wxArrayString& getUpdateStats() const { const wxArrayString& ref = updates; return ref; };
  const wxArrayString& getLatencyStats() const { const wxArrayString& ref = latencies; return ref; };
  const wxArrayString& getMCLossRatioStats() const { const wxArrayString& ref = mc_lossratios; return ref; };

  // cuttext
  const wxString& getCuttext() const { const wxString& ref = cuttext; return ref; };
  void setCuttext(const wxString& text) { wxCriticalSectionLocker lock(mutex_cuttext); cuttext = text; };

  wxBitmap getFrameBufferRegion(const wxRect& region) const;
  int getFrameBufferWidth() const;
  int getFrameBufferHeight() const;

  wxString getDesktopName() const;
  wxString getServerName() const;
  wxString getServerAddr() const;
  wxString getServerPort() const;
 
  // get error string
  const wxString& getErr() const { const wxString& ref = err; return ref; };
  // get global log string
  static const wxArrayString& getLog() { const wxArrayString& ref = log; return ref; };
  static void doLogfile(bool yesno) { do_logfile = yesno; };

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
