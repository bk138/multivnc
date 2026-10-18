#include "MyDialogHelp.h"


MyDialogHelp::MyDialogHelp(wxWindow* parent): DialogHelp(parent, wxID_ANY, _("MultiVNC Help"))
{
    // omit the empty space wxToolbook reserves for tool icons
    notebook_help->GetToolBar()->SetWindowStyleFlag(notebook_help->GetToolBar()->GetWindowStyleFlag() | wxTB_NOICONS);

    const auto add_page = [this](const wxString& title, const wxArrayString& paragraphs) {
        wxPanel* page = new wxPanel(notebook_help, wxID_ANY);
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        for (size_t i = 0; i < paragraphs.GetCount(); ++i) {
#if wxCHECK_VERSION(3,3,0)
            wxStaticText* label = new wxStaticText(page, wxID_ANY, paragraphs[i], wxDefaultPosition, wxDefaultSize, wxST_WRAP);
#else
            // wxST_WRAP needs wx >= 3.3, wrap at a fixed width instead
            wxStaticText* label = new wxStaticText(page, wxID_ANY, paragraphs[i]);
            label->Wrap(FromDIP(480));
#endif
            sizer->Add(label, 0, wxALL|wxEXPAND, 6);
        }
        page->SetSizer(sizer);
        notebook_help->AddPage(page, title);
    };

    add_page(_("Getting Started"), {_("If there are VNC servers advertising themselves via ZeroConf, you can select a host in the 'Available VNC Servers' list. Otherwise, use the 'Connect' button or menu item."),
                                     _("Each open connection is shown as a tab. A blue icon on the tab label means you are running in unicast mode, a green icon means you are running in multicast mode."),
                                     _("Connections can be added to the bookmarks list via the 'Bookmarks' menu and opened again later from there.")});

    add_page(_("Connecting"), {_("In the 'Connect' dialog, enter the host name or IP address of the VNC server and, optionally, its port. If no port is given, the default port 5900 is used."),
                               _("Check 'SSH Tunnel' to tunnel the connection through an SSH server. In this case, set the VNC host and port as reachable from the SSH server, e.g. localhost with the default port when the VNC server runs on the same machine as the SSH server. Authentication is done with a password or a private key."),
                               _("When connecting through an UltraVNC repeater, set host and port to the repeater and enter the repeater's session ID in the 'Session ID' field."),
                               _("The security types available to a connection depend on the build. See the 'About' dialog for the list of supported types.")});

    add_page(_("Listen Mode"), {_("Listen mode, also known as Reverse VNC, lets a VNC server connect to you instead of you connecting to the server."),
                                _("When you start listening, a new tab shows the port that MultiVNC listens on, starting with 5500. The VNC server has to connect to this machine on that port."),
                                _("You can listen on multiple ports at the same time, one per tab.")});

    add_page(_("Macros and Multi-Sync"), {_("'Record Input' records all mouse and keyboard input of the current connection. Stop recording to save the input to a file for later replay."),
                                          _("'Replay Input' replays a previously recorded macro. Hold Shift while clicking it to repeat the macro in a loop."),
                                          _("'Multi-Sync Input' replicates mouse and keyboard input to all open connections.")});

    SetMinSize(FromDIP(wxSize(480, 320)));
    SetSize(FromDIP(wxSize(800, 450)));
    Centre();
}
