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
DECLARE_EVENT_TYPE(VNCConnDisconnectNOTIFY, -1)
DECLARE_EVENT_TYPE(VNCConnUpdateNOTIFY, -1)
DECLARE_EVENT_TYPE(VNCConnFBResizeNOTIFY, -1)


class VNCConn: public wxEvtHandler
{
  friend class VNCThread;
  void *vncthread;

  void *parent;

  rfbClient* cl;

  // this counts the ms since Init()
  wxStopWatch conn_stopwatch;

  // complete framebuffer
  wxBitmap* framebuffer;
  wxAlphaPixelData* fb_data;

  // statistics
  bool do_stats;
  int updates_count;
  wxTimer updates_count_timer; // a timer to reset draw_count periodically
  void onUpdatesCountTimer(wxTimerEvent& event);
  wxPoint pointer_pos;
  wxStopWatch pointer_stopwatch;
  // string arrays to store values over time
  wxArrayString updates;
  wxArrayString latencies;


  // per-connection error string
  wxString err;
  
  // global libvcnclient log stuff
  // there's no per-connection log since we cannot find out which client
  // called the logger function :-(
  static wxArrayString log;
  static wxCriticalSection mutex_log;
  static bool do_logfile;

 
  void SendDisconnectNotify();
  // NB: this sets the event's clientdata ptr a newly created wRect 
  // which MUST be freed by its receiver!!!
  void SendUpdateNotify(int x, int y, int w, int h);
  void SendFBResizeNotify();


  //callbacks
  static rfbBool alloc_framebuffer(rfbClient* client);
  static void got_update(rfbClient* cl,int x,int y,int w,int h);
  static void kbd_leds(rfbClient* cl, int value, int pad);
  static void textchat(rfbClient* cl, int value, char *text);
  static void got_selection(rfbClient *cl, const char *text, int len);
  static void logger(const char *format, ...);


protected:
  DECLARE_EVENT_TABLE();


public:
  VNCConn(void *parent);
  ~VNCConn(); 

  bool Init(const wxString& host, char* (*getpasswdfunc)(rfbClient*), 
	    int compresslevel = 1, int quality = 5);
  bool Shutdown();

  bool sendPointerEvent(wxMouseEvent &event);
  bool sendKeyEvent(wxKeyEvent &event, bool down, bool isChar);
  
  // toggle statistics, default is off
  void doStats(bool yesno);
  // this clears internal statistics
  void resetStats();
  // get stats, in format "timestamp, value"
  const wxArrayString& getUpdateStats() const { const wxArrayString& ref = updates; return ref; };
  const wxArrayString& getLatencyStats() const { const wxArrayString& ref = latencies; return ref; };

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


#endif
