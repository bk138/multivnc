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

    void setShowAdvanced(bool yesno);
    bool getShowAdvanced();
};

#endif
