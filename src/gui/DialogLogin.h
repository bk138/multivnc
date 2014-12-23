// -*- C++ -*- 

#ifndef FORMLOGIN_H
#define FORMLOGIN_H
 
#include "wx/wx.h"
 
class DialogLogin : public wxDialog
{
 public:
    DialogLogin(wxFrame *parent, wxWindowID id, const wxString &title);
 
    // Destructor
    virtual ~DialogLogin();

    const wxString& getUserName() const { const wxString& ref = m_usernameEntry->GetValue(); return ref; };
    const wxString& getPassword() const { const wxString& ref = m_passwordEntry->GetValue(); return ref; };
 
 private:
    wxStaticText* m_usernameLabel;
    wxStaticText* m_passwordLabel;
    wxTextCtrl* m_usernameEntry;
    wxTextCtrl* m_passwordEntry;
    wxButton* m_buttonLogin;
    wxButton* m_buttonCancel;
 
 
 private:
    void OnCancel(wxCommandEvent& event);
    void OnLogin(wxCommandEvent& event);
 
 private:
    DECLARE_EVENT_TABLE()
 
	enum
	{
	    BUTTON_Login = wxID_HIGHEST + 1
	};
};
#endif // FORMLOGIN_H
