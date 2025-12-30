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
    //TODO privkey

    void setShowAdvanced(bool yesno);
    bool getShowAdvanced();

  private:
    void OnPasswordPrivkeyRadioSelected(wxCommandEvent& event);
};

#endif
