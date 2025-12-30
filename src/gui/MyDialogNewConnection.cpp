
#include "MyDialogNewConnection.h"
#include <wx/valnum.h>

MyDialogNewConnection::MyDialogNewConnection(wxWindow *parent, int id,
                                             const wxString &title,
                                             const wxPoint &pos,
                                             const wxSize &size, long style)
    : DialogNewConnection(parent, id, title, pos, size, style) {
    text_ctrl_repeater_id->SetValidator(wxIntegerValidator<int>());
    text_ctrl_ssh_port->SetValidator(wxIntegerValidator<int>(NULL, 0, 65535));
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

void MyDialogNewConnection::setShowAdvanced(bool yesno) {
    coll_pane_advanced->Collapse(!yesno);
};

bool MyDialogNewConnection::getShowAdvanced() {
    return coll_pane_advanced->IsExpanded();
};
