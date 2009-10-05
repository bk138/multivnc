
#include <wx/config.h>
#include "MyDialogSettings.h"
#include "../dfltcfg.h"




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
  pConfig->Read(K_COMPRESSLEVEL, &value, V_COMPRESSLEVEL);
  slider_compresslevel->SetValue(value);
  pConfig->Read(K_QUALITY, &value, V_QUALITY);
  slider_quality->SetValue(value);
  pConfig->Read(K_STATSAUTOSAVE, &value, V_STATSAUTOSAVE);
  checkbox_stats_save->SetValue(value);
  pConfig->Read(K_LOGSAVETOFILE, &value, V_LOGSAVETOFILE);
  checkbox_logfile->SetValue(value);
}


MyDialogSettings::~MyDialogSettings()
{
  

}




