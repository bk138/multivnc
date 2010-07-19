// -*- C++ -*- 

#ifndef MYDIALOGSETTINGS_H
#define MYDIALOGSETTINGS_H


#include "DialogSettings.h"


class MyDialogSettings: public DialogSettings
{

public:
 
  MyDialogSettings(wxWindow* parent, int id, const wxString& title, const wxPoint& pos=wxDefaultPosition, const wxSize& size=wxDefaultSize, long style=wxDEFAULT_DIALOG_STYLE);

  ~MyDialogSettings();
  
  int getCompressLevel() const { return slider_compresslevel->GetValue(); };
  int getQuality() const { return slider_quality->GetValue(); };
  bool getStatsAutosave() const { return checkbox_stats_save->GetValue(); };
  bool getLogSavetofile() const { return checkbox_logfile->GetValue(); };
  bool getDoMulticast() const { return checkbox_multicast->GetValue(); };
  bool getDoMulticastNACK() const { return !checkbox_multicastNACK->GetValue(); };
  int getMulticastRecvBuf() const { return slider_recvbuf->GetValue(); };
  bool getDoFastRequest() const { return checkbox_fastrequest->GetValue(); };
  int getFastRequestInterval() const { return slider_fastrequest->GetValue(); };
};



#endif
