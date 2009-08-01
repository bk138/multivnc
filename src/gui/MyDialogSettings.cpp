
#include "wx/bookctrl.h"
#include "wx/config.h"

#include "MyDialogSettings.h"
#include "../dfltcfg.h"

using namespace std;


MyDialogSettings::MyDialogSettings(wxWindow* parent, int id, const wxString& title, const wxPoint& pos, const wxSize& size, long style):
  wxPropertySheetDialog(parent, id, title, pos, size, style)
{
  CreateButtons(wxOK | wxCANCEL);

  wxBookCtrlBase* notebook = GetBookCtrl();
  
  wxPanel* clientSettings = CreateClientSettingsPage(notebook);

  
  notebook->AddPage(clientSettings, _("Client"), true);
  

   
  LayoutDialog();
}


MyDialogSettings::~MyDialogSettings()
{


}




wxPanel* MyDialogSettings::CreateClientSettingsPage(wxWindow* parent)
{
  wxConfigBase *pConfig = wxConfigBase::Get();
  
  wxPanel* panel = new wxPanel(parent, wxID_ANY);
  /*
  wxBoxSizer *topSizer = new wxBoxSizer( wxVERTICAL );
  wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );
  
  //// client choices
  wxString clientChoices[2];
  clientChoices[0] = _("&Remote Control only");
  clientChoices[1] = _("Remote Control with &Viewer");

  clientChoice = new wxRadioBox(panel, wxID_ANY, _("Client mode:"),
				wxDefaultPosition, wxDefaultSize, 2, clientChoices);
  clientChoice->SetToolTip(_("Choose Client Mode"));

  wxString clientMode;
  pConfig->Read(K_CLIENTMODE, &clientMode, V_DFLTCLIENTMODE);
  if(clientMode.IsSameAs(V_RCONLY))
    clientChoice->SetSelection(0);

  if(clientMode.IsSameAs(V_VIEWER))
    clientChoice->SetSelection(1);

  item0->Add(clientChoice, 0, wxGROW|wxALL, 5);
 

  //// custom clients

  wxBoxSizer* clientcustomSizer = new wxStaticBoxSizer(wxVERTICAL, panel,_("Custom clients:")  );



  wxStaticText *labelRConly = new wxStaticText(panel, wxID_ANY, _("Remote control only client"));
  clientcustomSizer->Add(labelRConly, 0, wxEXPAND|wxTOP|wxLEFT|wxRIGHT, 5);
  customClientRConly = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, 20));
  customClientRConly->SetValue(pConfig->Read(K_CUSTOMRCONLY, wxEmptyString));
  customClientRConly->SetToolTip(_("Specify a client program. %a and %p will be replaced by address and port to connect to."));
  clientcustomSizer->Add(customClientRConly, 0, wxEXPAND|wxALL, 5);



  wxStaticText *labelViewer = new wxStaticText(panel, wxID_ANY, _("Remote control with Viewer client"));
  clientcustomSizer->Add(labelViewer, 0, wxEXPAND|wxTOP|wxLEFT|wxRIGHT, 5);
  customClientViewer = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, 20));
  customClientViewer->SetValue(pConfig->Read(K_CUSTOMVIEWER, wxEmptyString));
  customClientViewer->SetToolTip(_("Specify a client program. %a and %p will be replaced by address and port to connect to."));
  clientcustomSizer->Add(customClientViewer, 0, wxEXPAND|wxALL, 5);


  item0->Add(clientcustomSizer, 0, wxEXPAND|wxLEFT|wxRIGHT, 5);


  // finalize
  topSizer->Add( item0, 1, wxGROW|wxALIGN_CENTRE|wxALL, 5 );
  topSizer->AddSpacer(5);

  panel->SetSizer(topSizer);
  topSizer->Fit(panel);
  */
  return panel;
}



