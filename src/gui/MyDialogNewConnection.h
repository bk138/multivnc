#ifndef MYDIALOGNEWCONNECTION_H
#define MYDIALOGNEWCONNECTION_H

#include "DialogNewConnection.h"
#include <wx/secretstore.h>

class MyDialogNewConnection : public DialogNewConnection {
  public:
    MyDialogNewConnection(wxWindow *parent, int id, const wxString &title,
                          const wxPoint &pos = wxDefaultPosition,
                          const wxSize &size = wxDefaultSize,
                          long style = wxDEFAULT_DIALOG_STYLE);

    wxString getHost() const;
    void setHost(const wxString &host);

    int getRepeaterId() const;

    wxString getSshServer() const;
    int getSshPort() const;
    wxString getSshUser() const;
    wxSecretValue getSshPassword() const;
    wxString getSshPrivKeyFilename() const;
    wxSecretValue getSshPrivkeyPassword() const;

    void setShowAdvanced(bool yesno);
    bool getShowAdvanced();

  private:
    wxString mSshPrivKeyFilename;

    void OnPasswordPrivkeyRadioSelected(wxCommandEvent &event);
    void OnPrivkeyFileOpen(wxCommandEvent& event);
};

#endif
