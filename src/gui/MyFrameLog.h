// -*- C++ -*- 

#ifndef MYFRAMELOG_H
#define MYFRAMELOG_H


#include "FrameLog.h"



// make available custom close event
DECLARE_EVENT_TYPE(MyFrameLogCloseNOTIFY, -1)

class MyFrameLog: public FrameLog
{
  friend class LogThread;
  void *logthread;

  void SendCloseNotify();

  
public:
  MyFrameLog(wxWindow* parent, int id, const wxString& title, 
	     const wxPoint& pos=wxDefaultPosition, 
	     const wxSize& size=wxDefaultSize, 
	     long style=wxDEFAULT_FRAME_STYLE);

  ~MyFrameLog();

  void log_saveas(wxCommandEvent &event); 
  void log_close(wxCommandEvent &event);
};

#endif
