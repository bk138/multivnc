// -*- C++ -*- 

#ifndef MYDIALOGSETTINGS_H
#define MYDIALOGSETTINGS_H

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/propdlg.h"


class MyDialogSettings: public wxPropertySheetDialog
{
  wxRadioBox *clientChoice;
  wxTextCtrl *customClientRConly;
  wxTextCtrl *customClientViewer;


  wxPanel* CreateClientSettingsPage(wxWindow* parent);

public:
 

 
  
  MyDialogSettings(wxWindow* parent, int id, const wxString& title, const wxPoint& pos=wxDefaultPosition, const wxSize& size=wxDefaultSize, long style=wxDEFAULT_DIALOG_STYLE);


  ~MyDialogSettings();
  
   
 
};



#endif
