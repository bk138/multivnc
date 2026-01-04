
#include "MyDialogNewConnection.h"
#include <wx/file.h>
#include <wx/log.h>
#include <wx/valnum.h>

MyDialogNewConnection::MyDialogNewConnection(wxWindow *parent, int id,
                                             const wxString &title,
                                             const wxPoint &pos,
                                             const wxSize &size, long style)
    : DialogNewConnection(parent, id, title, pos, size, style) {
    text_ctrl_repeater_id->SetValidator(wxIntegerValidator<int>());
    text_ctrl_ssh_port->SetValidator(wxIntegerValidator<int>(NULL, 0, 65535));

    // Bind radio button events
    radio_btn_ssh_password->Bind(wxEVT_RADIOBUTTON,
                                 &MyDialogNewConnection::OnPasswordPrivkeyRadioSelected, this);
    radio_btn_ssh_privkey->Bind(wxEVT_RADIOBUTTON,
                                &MyDialogNewConnection::OnPasswordPrivkeyRadioSelected, this);

    // and privkey open dialog
    button_ssh_privkey_open->Bind(wxEVT_BUTTON, &MyDialogNewConnection::OnPrivkeyFileOpen, this);
};

wxString MyDialogNewConnection::getHost() const {
    return text_ctrl_host->GetValue();
};

void MyDialogNewConnection::setHost(const wxString &host) {
    text_ctrl_host->SetValue(host);
};

int MyDialogNewConnection::getRepeaterId() const {
    int value;
    return text_ctrl_repeater_id->GetValue().ToInt(&value) ? value : -1;
};

wxString MyDialogNewConnection::getSshServer() const {
    return text_ctrl_ssh_server->GetValue();
};

int MyDialogNewConnection::getSshPort() const {
    int value;
    return text_ctrl_ssh_port->GetValue().ToInt(&value) ? value : -1;
};

wxString MyDialogNewConnection::getSshUser() const {
    return text_ctrl_ssh_user->GetValue();
};

wxSecretValue MyDialogNewConnection::getSshPassword() const {
    return wxSecretValue(text_ctrl_ssh_password->GetValue());
};

wxString MyDialogNewConnection::getSshPrivKeyFilename() const {
    return mSshPrivKeyFilename;
}

wxSecretValue MyDialogNewConnection::getSshPrivkeyPassword() const {
    return wxSecretValue(text_ctrl_ssh_privkey_password->GetValue());
}

void MyDialogNewConnection::setShowAdvanced(bool yesno) {
    coll_pane_advanced->Collapse(!yesno);
};

bool MyDialogNewConnection::getShowAdvanced() {
    return coll_pane_advanced->IsExpanded();
};

void MyDialogNewConnection::OnPasswordPrivkeyRadioSelected(wxCommandEvent &event) {
    if (event.GetEventObject() == radio_btn_ssh_password) {
        //password
        label_ssh_password->Show();
        text_ctrl_ssh_password->Show();
        // privkey
        button_ssh_privkey_open->Hide();
        label_ssh_privkey_password->Hide();
        text_ctrl_ssh_privkey_password->Hide();
        button_ssh_privkey_open->Hide();
    } else if (event.GetEventObject() == radio_btn_ssh_privkey) {
        //password
        label_ssh_password->Hide();
        text_ctrl_ssh_password->Hide();
        // privkey
        button_ssh_privkey_open->Show();
        label_ssh_privkey_password->Show();
        text_ctrl_ssh_privkey_password->Show();
        button_ssh_privkey_open->Show();
    }
    panel_advanced->GetSizer()->SetSizeHints(panel_advanced);
    GetSizer()->SetSizeHints(this);
    Layout();
}

void MyDialogNewConnection::OnPrivkeyFileOpen(wxCommandEvent &event) {
    wxString filename = wxFileSelector("Choose an SSH private key file");

    if (!filename.empty()) {
        wxBusyCursor busy;
        wxFile file;

        if (!file.Open(filename)) {
            wxLogError(_("Failed to open file %s."), filename);
            return;
        }

        wxFileOffset fileSize = file.Length();

        if (fileSize == 0) {
            wxLogError(_("File %s is empty."), filename);
            return;
        }

        if (fileSize > 100 * 1024) {
            wxLogError("File %s is too large.", filename);
            return;
        }

        mSshPrivKeyFilename = filename;
    }
}
