#ifndef MYDIALOGNEWCONNECTION_H
#define MYDIALOGNEWCONNECTION_H

#include "DialogNewConnection.h"

class MyDialogNewConnection : public DialogNewConnection {
public:
    MyDialogNewConnection(wxWindow *parent, int id, const wxString &title,
                          const wxPoint &pos = wxDefaultPosition,
                          const wxSize &size = wxDefaultSize,
                          long style = wxDEFAULT_DIALOG_STYLE)
        : DialogNewConnection(parent, id, title, pos, size, style){};

    wxString getHost() const { return text_ctrl_host->GetValue(); };
    void setHost(const wxString &host) { text_ctrl_host->SetValue(host); };

    int getRepeaterId() const {
        int value;
        return text_ctrl_repeater_id->GetValue().ToInt(&value) ? value : -1;
    };

    void setShowAdvanced(bool yesno) { coll_pane_advanced->Collapse(!yesno); };
    bool getShowAdvanced() { return coll_pane_advanced->IsExpanded(); };
};

#endif
