
#include "wx/thread.h"
#include "VNCConn.h"
#include "MyFrameLog.h"



DEFINE_EVENT_TYPE(MyFrameLogCloseNOTIFY)

// this is used internally as we cannot update the 
// txtctrl from another thread without crashing
DECLARE_EVENT_TYPE(MyFrameLogUpdateNOTIFY, -1)
DEFINE_EVENT_TYPE(MyFrameLogUpdateNOTIFY)


// map recv of custom events to handler methods
BEGIN_EVENT_TABLE(MyFrameLog, FrameLog)
  EVT_COMMAND (wxID_ANY, MyFrameLogUpdateNOTIFY, MyFrameLog::onUpdate)
END_EVENT_TABLE()


/********************************************

  internal worker thread class

********************************************/

class LogThread : public wxThread
{
  MyFrameLog *p;    // our parent object
 
public:
  LogThread(MyFrameLog *parent);
  
  // thread execution starts here
  virtual wxThread::ExitCode Entry();

};


LogThread::LogThread(MyFrameLog *parent)
  : wxThread()
{
  p = parent;
}


wxThread::ExitCode LogThread::Entry()
{
  while(! TestDestroy()) 
    {
      if(p->lines_printed < VNCConn::getLog().GetCount())
	p->SendUpdateNotify();

      wxSleep(1);
    }
}




/********************************************

  MyFrameLog class

********************************************/

/*
  constructor/destructor
*/

MyFrameLog::MyFrameLog(wxWindow* parent, int id, const wxString& title, const wxPoint& pos, const wxSize& size, long style):
  FrameLog(parent, id, title, pos, size, style)
{
  lines_printed = 0;

  // this is like our main loop
  LogThread *tp = new LogThread(this);
  // save it for later on
  logthread = tp;
  
  if( tp->Create() != wxTHREAD_NO_ERROR )
    {
      wxLogError(_("Could not create log window!"));
      // this seems ok, as it emits a close event, which in turn
      // destroys the just created log window, well well well...
      Close();
    }
  else
    tp->Run();
}


MyFrameLog::~MyFrameLog()
{
  static_cast<LogThread*>(logthread)->Delete();
  SendCloseNotify();
}




/*
  private members
*/

void MyFrameLog::SendCloseNotify()
{
  // new NOTIFY event, we got no window id
  wxCommandEvent event(MyFrameLogCloseNOTIFY, wxID_ANY);
  event.SetEventObject(this); // set sender

  // Send it
  wxPostEvent((wxEvtHandler*)GetParent(), event);
}


void MyFrameLog::SendUpdateNotify()
{
  // new NOTIFY event, we got no window id
  wxCommandEvent event(MyFrameLogUpdateNOTIFY, wxID_ANY);
  event.SetEventObject(this); // set sender

  // Send it
  wxPostEvent((wxEvtHandler*)this, event);
}


void MyFrameLog::onUpdate(wxCommandEvent& event)
{
  wxArrayString log = VNCConn::getLog();

  while(lines_printed < log.GetCount())
    {
      *text_ctrl_log << log[lines_printed];
      ++lines_printed;
    }
}







/*
  public members
*/

void MyFrameLog::log_saveas(wxCommandEvent &event)
{
  wxString filename = wxFileSelector(_("Save log as..."), _T(""), _T("saved_log.txt"), _T(""),
				     _T("*.*"), wxFD_SAVE);
  if(wxFileExists(filename))
    if(wxMessageBox(_("File already exists. Overwrite it?"), _("Overwrite?"), wxYES_NO|wxICON_QUESTION, this) == wxNO)
      return;

  wxBusyCursor busy;

  text_ctrl_log->SaveFile(filename);
}
 


void MyFrameLog::log_close(wxCommandEvent &event)
{
  Close();
}




