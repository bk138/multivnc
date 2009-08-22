
#include <cstdarg>
#include <ctime>
#include "wx/thread.h"
#include "wx/intl.h"

#include "VNCConn.h"


// use some global address
#define VNCCONN_OBJ_ID VNCConn::got_update

// logfile name
#define LOGFILE _T("MultiVNC.log")


// define our new notify event!
DEFINE_EVENT_TYPE(VNCConnDisconnectNOTIFY)



/*
  internal worker thread class
*/
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
      // if(got some input  event)
      //handleEvent(cl, &e);
      //else 
	{
	  i=WaitForMessage(p->cl,500);
	  if(i<0)
	    {
	      p->SendDisconnectNotify();
	      return 0;
	    }
	  if(i)
	    if(!HandleRFBServerMessage(p->cl))
	      {
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
}








void VNCConn::SendDisconnectNotify()
{
  // new NOTIFY event, we got no window id
  wxCommandEvent event(VNCConnDisconnectNOTIFY, wxID_ANY);
  event.SetEventObject(this); // set sender

  // Send it
  wxPostEvent((wxEvtHandler*)parent, event);
}







VNCConn::VNCConn(void* p)
{
  // save our caller
  parent = p;
  
  vncthread = cl = 0;
  rfbClientLog = rfbClientErr = logger;
}




VNCConn::~VNCConn()
{
  Shutdown();
}







void VNCConn::got_update(rfbClient* cl,int x,int y,int w,int h)
{
  //fprintf(stderr, "got update\n");
}



rfbBool VNCConn::resize(rfbClient* client)
{

}




void VNCConn::kbd_leds(rfbClient* cl, int value, int pad)
{
  fprintf(stderr,"Led State= 0x%02X\n", value);
  fflush(stderr);
}

void VNCConn::textchat(rfbClient* cl, int value, char *text)
{

}
void VNCConn::got_selection(rfbClient *cl, const char *text, int len)
{

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




bool VNCConn::Init(const wxString& host, char* (*getpasswdfunc)(rfbClient*))
{
  Shutdown();

  int argc = 2;
  char* argv[argc];
  argv[0] = strdup("VNCConn");
  argv[1] = strdup(host.mb_str());

 
  // 16-bit: 
  //cl=rfbGetClient(5,3,2); 
  cl=rfbGetClient(8,3,4);

  rfbClientSetClientData(cl, (void*)VNCCONN_OBJ_ID, this); 
  
  // callbacks
  //cl->MallocFrameBuffer = resize;
  cl->GotFrameBufferUpdate = got_update;
  cl->GetPassword = getpasswdfunc;
  cl->HandleKeyboardLedState = kbd_leds;
  cl->HandleTextChat = textchat;
  cl->GotXCutText = got_selection;

  //cl->canHandleNewFBSize = TRUE;
  
  if(! rfbInitClient(cl, &argc, argv))
    {
      cl = 0; //  rfbInitClient() calls rfbClientCleanup() on failure, but this does not zero the ptr
      err.Printf(_("Failure connecting to server!"));
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
  VNCThread* tp = (VNCThread*) vncthread;

  if(tp)
    {
      wxCriticalSectionLocker lock(mutex_vncthread);
      tp->Delete();
      tp = 0;
    }
  
  if(cl)
    {
      rfbClientCleanup(cl);
      cl = 0;
    }
}






