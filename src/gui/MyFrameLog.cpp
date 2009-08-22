
#include "wx/thread.h"
#include "VNCConn.h"
#include "MyFrameLog.h"



DEFINE_EVENT_TYPE(MyFrameLogCloseNOTIFY)




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
  size_t line_nr = 0; 

  while(! TestDestroy()) 
    {
      wxArrayString log = VNCConn::getLog();
   
      while(line_nr < log.GetCount())
	{
	  *(p->text_ctrl_log) << log[line_nr];
	  ++line_nr;
	}

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




