// -*- C++ -*- 

#ifndef VNCCONN_H
#define VNCCONN_H

#include "wx/event.h"
#include "wx/string.h"
#include "wx/arrstr.h"
#include "wx/bitmap.h"
#include "wx/rawbmp.h"
#include <rfb/rfbclient.h>



// make available custom events
DECLARE_EVENT_TYPE(VNCConnDisconnectNOTIFY, -1)
DECLARE_EVENT_TYPE(VNCConnUpdateNOTIFY, -1)


class VNCConn: public wxObject
{
  friend class VNCThread;
  void *vncthread;

  void *parent;

  rfbClient* cl;

  // complete framebuffer
  wxBitmap* framebuffer;
  wxAlphaPixelData* fb_data;


  // per-connection error string
  wxString err;
  
  // global libvcnclient log stuff
  // there's no per-connection log since we cannot find out which client
  // called the logger function :-(
  static wxArrayString log;
  static wxCriticalSection mutex_log;

 
  void SendDisconnectNotify();
  // NB: this sets the event's clientdata ptr a newly created wRect 
  // which MUST be freed by its receiver!!!
  void SendUpdateNotify(int x, int y, int w, int h);


  //callbacks
  static rfbBool alloc_framebuffer(rfbClient* client);
  static void got_update(rfbClient* cl,int x,int y,int w,int h);
  static void kbd_leds(rfbClient* cl, int value, int pad);
  static void textchat(rfbClient* cl, int value, char *text);
  static void got_selection(rfbClient *cl, const char *text, int len);
  static void logger(const char *format, ...);



public:
  VNCConn(void *parent);
  ~VNCConn(); 

  bool Init(const wxString& host, char* (*getpasswdfunc)(rfbClient*), int compresslevel = 1, int quality = 5);
  bool Shutdown();
 
  wxBitmap getFrameBufferRegion(const wxRect& region) const;
  int getFrameBufferWidth() const;
  int getFrameBufferHeight() const;

  const wxString getServerName() const;
  const wxString getServerAddr() const;
  const wxString getServerPort() const;
 
  // get error string
  const wxString& getErr() const { const wxString& ref = err; return ref; };
  // get gloabl log string
  static const wxArrayString& getLog() { const wxArrayString& ref = log; return ref; };


};


#endif
