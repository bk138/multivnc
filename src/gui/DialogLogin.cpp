#include "DialogLogin.h"
#include <wx/wx.h>
#include <wx/statline.h>

/*
  adopted from http://imron02.wordpress.com/2014/09/26/c-simple-form-login-using-wxwidgets/, thanks!
 */

DialogLogin::DialogLogin(wxFrame *parent, wxWindowID id, const wxString &title )
: wxDialog(parent, id, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) 
{
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
 
    wxBoxSizer *hbox1 = new wxBoxSizer(wxHORIZONTAL);
    m_usernameLabel = new wxStaticText(this, wxID_ANY, _("Username:") + " ", wxDefaultPosition, wxSize(110, -1));
    hbox1->Add(m_usernameLabel, 0);
 
    m_usernameEntry = new wxTextCtrl(this, wxID_ANY);
    hbox1->Add(m_usernameEntry, 1);
    vbox->Add(hbox1, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
 
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    m_passwordLabel = new wxStaticText(this, wxID_ANY, _("Password:") + " ", wxDefaultPosition, wxSize(110, -1));
    hbox2->Add(m_passwordLabel, 0);
 
    m_passwordEntry = new wxTextCtrl(this, BUTTON_Login, wxString(""),
        wxDefaultPosition, wxSize(220, -1), wxTE_PASSWORD|wxTE_PROCESS_ENTER);
    hbox2->Add(m_passwordEntry, 1);
    vbox->Add(hbox2, 0, wxEXPAND | wxLEFT | wxTOP | wxRIGHT, 10);

    // divider
    wxStaticLine *divider = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL);
    vbox->Add(divider, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

    // buttons
    wxBoxSizer *hbox3 = new wxBoxSizer(wxHORIZONTAL);
    m_buttonCancel = new wxButton(this, wxID_CANCEL, _("Cancel"));
    hbox3->Add(m_buttonCancel, 1, wxEXPAND | wxALL, 3);
    m_buttonLogin = new wxButton(this, BUTTON_Login, _("Login"));
    hbox3->Add(m_buttonLogin, 1, wxEXPAND | wxALL, 3);
    vbox->Add(hbox3, 0, wxEXPAND | wxALL, 10);

    SetSizerAndFit(vbox);
    
}
 
void DialogLogin::OnCancel(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
}
 
void DialogLogin::OnLogin(wxCommandEvent& event)
{
    wxString username = m_usernameEntry->GetValue();
    wxString password = m_passwordEntry->GetValue();
 
    if (username.empty() || password.empty()) {
        wxMessageBox(_("Username or password must not empty"), _("Warning!"), wxICON_WARNING);
    }
    else
	EndModal(wxID_OK);
  
}
 
DialogLogin::~DialogLogin() {}
 
BEGIN_EVENT_TABLE(DialogLogin, wxDialog)
EVT_BUTTON(wxID_CANCEL, DialogLogin::OnCancel)
EVT_BUTTON(BUTTON_Login, DialogLogin::OnLogin)
EVT_TEXT_ENTER(BUTTON_Login, DialogLogin::OnLogin)
END_EVENT_TABLE()
