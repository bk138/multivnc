// -*- C++ -*- 

#ifndef MYDIALOGSETTINGS_H
#define MYDIALOGSETTINGS_H


#include "DialogSettings.h"


class MyDialogSettings: public DialogSettings
{

public:
 
  MyDialogSettings(wxWindow* parent, int id, const wxString& title, const wxPoint& pos=wxDefaultPosition, const wxSize& size=wxDefaultSize, long style=wxDEFAULT_DIALOG_STYLE);

  ~MyDialogSettings();
  
  bool getStatsAutosave() const { return checkbox_stats_save->GetValue(); };
  bool getLogSavetofile() const { return checkbox_logfile->GetValue(); };
  bool getDoMulticast() const { return checkbox_multicast->GetValue(); };
  int getMulticastSocketRecvBuf() const { return slider_socketrecvbuf->GetValue(); };
  int getMulticastRecvBuf() const { return slider_recvbuf->GetValue(); };
  bool getDoFastRequest() const { return checkbox_fastrequest->GetValue(); };
  int getFastRequestInterval() const { return slider_fastrequest->GetValue(); };
  bool getQoS_EF() const { return checkbox_qos_ef->GetValue(); };

  bool getEncCopyRect() const { return checkbox_enc_copyrect->GetValue(); };
  bool getEncHextile() const { return checkbox_enc_hextile->GetValue(); };
  bool getEncRRE() const { return checkbox_enc_rre->GetValue(); };
  bool getEncCoRRE() const { return checkbox_enc_corre->GetValue(); };
  bool getEncZRLE() const { return checkbox_enc_zrle->GetValue(); };
  bool getEncZYWRLE() const { return checkbox_enc_zywrle->GetValue(); };
  bool getEncZlib() const { return checkbox_enc_zlib->GetValue(); };
  bool getEncZlibHex() const { return checkbox_enc_zlibhex->GetValue(); };
  bool getEncUltra() const { return checkbox_enc_ultra->GetValue(); };
  bool getEncTight() const { return checkbox_enc_tight->GetValue(); };

  int getCompressLevel() const { return slider_compresslevel->GetValue(); };
  int getQuality() const { return slider_quality->GetValue(); };
};



#endif
