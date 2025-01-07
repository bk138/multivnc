
#include <wx/config.h>
#include "MyDialogSettings.h"
#include "VNCConn.h"
#include "dfltcfg.h"




MyDialogSettings::MyDialogSettings(wxWindow* parent, int id, const wxString& title, const wxPoint& pos, const wxSize& size, long style):
  DialogSettings(parent, id, title, pos, size, style)
{
  // this creates standard buttons in platform specific order
  wxSizer* std_buttonsizer = CreateButtonSizer(wxOK | wxCANCEL);
  // Add it to the dialog's main sizer 
  GetSizer()->Add(std_buttonsizer, 0, wxEXPAND | wxALL, 3);
  // and resize to fit
  Fit();

  // get current settings
  int value;
  wxConfigBase *pConfig = wxConfigBase::Get();
  pConfig->Read(K_STATSAUTOSAVE, &value, V_STATSAUTOSAVE);
  checkbox_stats_save->SetValue(value);
  pConfig->Read(K_LOGSAVETOFILE, &value, V_LOGSAVETOFILE);
  checkbox_logfile->SetValue(value);
  pConfig->Read(K_MULTICAST, &value, V_MULTICAST);
  checkbox_multicast->SetValue(value);
  pConfig->Read(K_MULTICASTSOCKETRECVBUF, &value, V_MULTICASTSOCKETRECVBUF);
  slider_socketrecvbuf->SetValue(value);
  pConfig->Read(K_MULTICASTRECVBUF, &value, V_MULTICASTRECVBUF);
  slider_recvbuf->SetValue(value);
  pConfig->Read(K_FASTREQUEST, &value, V_FASTREQUEST);
  checkbox_fastrequest->SetValue(value);
  pConfig->Read(K_FASTREQUESTINTERVAL, &value, V_FASTREQUESTINTERVAL);
  slider_fastrequest->SetValue(value);
  pConfig->Read(K_QOS_EF, &value, V_QOS_EF);
  checkbox_qos_ef->SetValue(value);

  // adopt socket recv buf slider to OS-dependent max
  int os_dependent_max = VNCConn::getMaxSocketRecvBufSize();
  if(slider_socketrecvbuf->GetValue() > os_dependent_max)
      slider_socketrecvbuf->SetValue(os_dependent_max); // needed on MacOS at least
  if(slider_socketrecvbuf->GetMax() > os_dependent_max)
    slider_socketrecvbuf->SetRange(slider_socketrecvbuf->GetMin(), os_dependent_max);

  // read in encodings settings
  pConfig->Read(K_ENC_COPYRECT, &value, V_ENC_COPYRECT);
  checkbox_enc_copyrect->SetValue(value);
  pConfig->Read(K_ENC_RRE, &value, V_ENC_RRE);
  checkbox_enc_rre->SetValue(value);
  pConfig->Read(K_ENC_CORRE, &value, V_ENC_CORRE);
  checkbox_enc_corre->SetValue(value);
  pConfig->Read(K_ENC_ZRLE, &value, V_ENC_ZRLE);
  checkbox_enc_zrle->SetValue(value);
  pConfig->Read(K_ENC_ZYWRLE, &value, V_ENC_ZYWRLE);
  checkbox_enc_zywrle->SetValue(value);
  pConfig->Read(K_ENC_HEXTILE, &value, V_ENC_HEXTILE);
  checkbox_enc_hextile->SetValue(value);
  pConfig->Read(K_ENC_ZLIB, &value, V_ENC_ZLIB);
  checkbox_enc_zlib->SetValue(value);
  pConfig->Read(K_ENC_ZLIBHEX, &value, V_ENC_ZLIBHEX);
  checkbox_enc_zlibhex->SetValue(value);
  pConfig->Read(K_ENC_ULTRA, &value, V_ENC_ULTRA);
  checkbox_enc_ultra->SetValue(value);
  pConfig->Read(K_ENC_TIGHT, &value, V_ENC_TIGHT);
  checkbox_enc_tight->SetValue(value);

  // but only show encodings we support in this build
#ifndef LIBVNCSERVER_HAVE_LIBZ
  checkbox_enc_zrle->Disable();
  checkbox_enc_zrle->SetValue(false);
  checkbox_enc_zywrle->Disable();
  checkbox_enc_zywrle->SetValue(false);
  checkbox_enc_zlib->Disable();
  checkbox_enc_zlib->SetValue(false);
  checkbox_enc_zlibhex->Disable();
  checkbox_enc_zlibhex->SetValue(false);
  checkbox_enc_tight->Disable();
  checkbox_enc_tight->SetValue(false);
#endif
#ifndef LIBVNCSERVER_HAVE_LIBJPEG
  checkbox_enc_tight->Disable();
  checkbox_enc_tight->SetValue(false);
#endif


  pConfig->Read(K_COMPRESSLEVEL, &value, V_COMPRESSLEVEL);
  slider_compresslevel->SetValue(value);
  pConfig->Read(K_QUALITY, &value, V_QUALITY);
  slider_quality->SetValue(value);
}


