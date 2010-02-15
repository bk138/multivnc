
#include "MyFrameLog.h"
#include "VNCConn.h"



/********************************************

  MyFrameLog class

********************************************/

#define UPDATE_TIMER_INTERVAL 100

DEFINE_EVENT_TYPE(MyFrameLogCloseNOTIFY)


BEGIN_EVENT_TABLE(MyFrameLog, FrameLog)
    EVT_TIMER (wxID_ANY, MyFrameLog::onUpdateTimer)
END_EVENT_TABLE();



/*
  constructor/destructor
*/

MyFrameLog::MyFrameLog(wxWindow* parent, int id, const wxString& title, const wxPoint& pos, const wxSize& size, long style):
  FrameLog(parent, id, title, pos, size, style)
{
  lines_printed = 0;

  update_timer.SetOwner(this);
  update_timer.Start(UPDATE_TIMER_INTERVAL);
}


MyFrameLog::~MyFrameLog()
{
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




void MyFrameLog::onUpdateTimer(wxTimerEvent& event)
{
  wxBusyCursor wait;
  wxArrayString log = VNCConn::getLog();

  Freeze();
  while(lines_printed < log.GetCount())
    {
      *text_ctrl_log << log[lines_printed];
      ++lines_printed;
    }
  Thaw();
}




/*
  public members
*/

void MyFrameLog::log_saveas(wxCommandEvent &event)
{
  wxString filename = wxFileSelector(_("Save log as..."), wxEmptyString, wxT("saved_log.txt"), 
				     wxT(".txt"), _("Text files (*.txt)|*.txt"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if(! filename.empty())
    {
      wxBusyCursor busy;
      text_ctrl_log->SaveFile(filename);
    }
}
 

void MyFrameLog::log_close(wxCommandEvent &event)
{
  Close();
}


void MyFrameLog::log_clear(wxCommandEvent &event)
{
  text_ctrl_log->Clear();
  VNCConn::clearLog();
  lines_printed = 0;
}


