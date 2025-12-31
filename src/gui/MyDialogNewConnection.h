#ifndef MYDIALOGNEWCONNECTION_H
#define MYDIALOGNEWCONNECTION_H

#include "DialogNewConnection.h"

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
    wxString getSshPassword() const;
    std::vector<char> getSshPrivKey() const;
    wxString getSshPrivkeyPassword() const;

    void setShowAdvanced(bool yesno);
    bool getShowAdvanced();

  private:
    std::vector<char> mSshPrivKey;

    void OnPasswordPrivkeyRadioSelected(wxCommandEvent &event);
    void OnPrivkeyFileOpen(wxCommandEvent& event);
};

#endif
