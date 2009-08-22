// -*- C++ -*- 

#ifndef VNCCONN_H
#define VNCCONN_H

#include "wx/event.h"
#include "wx/string.h"
#include "wx/arrstr.h"
#include <rfb/rfbclient.h>



// make available custom disconnect event
DECLARE_EVENT_TYPE(VNCConnDisconnectNOTIFY, -1)


class VNCConn: public wxObject
{
  friend class VNCThread;
  void *vncthread;
  wxCriticalSection mutex_vncthread;    // protects the vncthread pointer

  void *parent;

  rfbClient* cl;

  // per-connection error string
  wxString err;
  
  // global libvcnclient log stuff
  // there's no per-connection log since we cannot find out which client
  // called the logger function :-(
  static wxArrayString log;
  static wxCriticalSection mutex_log;

 
  void SendDisconnectNotify();

  //callbacks
  static void got_update(rfbClient* cl,int x,int y,int w,int h);
  static rfbBool resize(rfbClient* client);
  static void kbd_leds(rfbClient* cl, int value, int pad);
  static void textchat(rfbClient* cl, int value, char *text);
  static void got_selection(rfbClient *cl, const char *text, int len);
  static void logger(const char *format, ...);



public:
  VNCConn(void *parent);
  ~VNCConn(); 

  bool Init(const wxString& host, char* (*getpasswdfunc)(rfbClient*));
  bool Shutdown();
 

  // get error string
  const wxString& getErr() const { const wxString& ref = err; return ref; };
  // get gloabl log string
  static const wxArrayString& getLog() { const wxArrayString& ref = log; return ref; };


};


#endif
