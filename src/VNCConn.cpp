
#include <cstdarg>
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

  while(! TestDestroy()) 
    {
      /* wxLogDebug( "VNCThread::Entry(): sleeping\n");
	      
	 wxSleep(1);

	 wxLogDebug( "VNCThread::Entry(): back again\n");*/

      // if(got some input  event)
      //handleEvent(cl, &e);
      //else 
      {
	i=WaitForMessage(p->cl, 500);
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



void VNCConn::SendUpdateNotify()
{
  // new NOTIFY event, we got no window id
  wxCommandEvent event(VNCConnUpdateNOTIFY, wxID_ANY);
  event.SetEventObject(this); // set sender

  // set info about what was updated
  event.SetClientData(&updated_region);

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
	  fb_it.Alpha() = 0;
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
 
  conn->updated_region.x = x;
  conn->updated_region.y = y;
  conn->updated_region.width = w;
  conn->updated_region.height = h;

  conn->SendUpdateNotify();
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


bool VNCConn::Init(const wxString& host, char* (*getpasswdfunc)(rfbClient*))
{
  Shutdown();

  int argc = 2;
  char* argv[argc];
  argv[0] = strdup("VNCConn");
  argv[1] = strdup(host.mb_str());

   
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



// its important to note that we need to pass a wxRect _copy_
// as updated_region can be changed by the vncthread 
// during execution of this function
wxBitmap VNCConn::getFrameBufferRegion(const wxRect rect) const
{
#ifdef __WXDEBUG__
  // save a picture only if the last is older than 5 seconds 
  static time_t t0 = 0,t1;
  t1 = time(NULL);
  if(t1 - t0 > 5)
    {
      t0 = t1;
      wxString fbdump( wxT("fb-dump-") + getServerName() + wxT(":") + getServerPort() + wxT(".bmp"));
      framebuffer->SaveFile(fbdump, wxBITMAP_TYPE_BMP);
      wxLogDebug(wxT("VNCConn %p: saved raw framebuffer to '%s'"), this, fbdump.c_str());
    }
#endif



#ifdef __WIN32__
  /* 
     windows stores DIB data in BGRA, not RGBA.
     and: DIBs store data from bottom to top :-(
  */
  
  // don't use getsubbitmap() here, instead
  // copy directly from framebuffer into a new bitmap,
  // saving one complete copy iteration
  wxBitmap region(rect.width, rect.height, cl->format.bitsPerPixel);

  wxAlphaPixelData region_data(region);
  wxAlphaPixelData::Iterator region_it(region_data);


  // get an wxAlphaPixelData from the framebuffer representing the subregion we want
  wxAlphaPixelData fbsub_data(*framebuffer, rect);
  if ( ! fbsub_data )
    {
      wxLogDebug(wxT("Failed to gain raw access to fb_sub data!"));
      return region;
    }
  wxAlphaPixelData::Iterator fbsub_it(fbsub_data);

  // and move iterator to the last row
  fbsub_it.OffsetY(fbsub_data, rect.height - 1);


  for( int y = 0; y < rect.height; ++y )
    {
      wxAlphaPixelData::Iterator region_it_rowStart = region_it;
      wxAlphaPixelData::Iterator fbsub_it_rowStart = fbsub_it;

      for( int x = 0; x < rect.width; ++x, ++region_it, ++fbsub_it )
	{
	  region_it.Red() = fbsub_it.Blue();
	  region_it.Green() = fbsub_it.Green();	  
	  region_it.Blue() = fbsub_it.Red();	  
	  region_it.Alpha() = fbsub_it.Alpha();	  
	}
      
      // this goes downwards
      region_it = region_it_rowStart;
      region_it.OffsetY(region_data, 1);

      // this goes upwards
      fbsub_it = fbsub_it_rowStart;
      fbsub_it.OffsetY(fbsub_data, - 1);
    }

  return region;

#else
  // RGBA top to bottom, as we like it...
  return framebuffer->GetSubBitmap(rect);


  // getsubbitmap makes a copy.
  // maybe find a way that just links the existing fb data into a new wxBitmap
  /*
  wxBitmap ret(rect.width, rect.height, cl->format.bitsPerPixel); 
  conn->framebuffer->GetRawData(*conn->fb_data, cl->format.bitsPerPixel);
  */
#endif
 
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


const wxString VNCConn::getServerName() const
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

const wxString VNCConn::getServerAddr() const
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

const wxString VNCConn::getServerPort() const
{
  if(cl)
    return wxString() << cl->serverPort;
  else
    return wxEmptyString;
}
