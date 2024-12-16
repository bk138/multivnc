// -*- C++ -*-
//
// generated by wxGlade 1.0.4 on Mon Dec 16 21:25:53 2024
//
// Example for compiling a single file project under Linux using g++:
//  g++ MyApp.cpp $(wx-config --libs) $(wx-config --cxxflags) -o MyApp
//
// Example for compiling a multi file project under Linux using g++:
//  g++ main.cpp $(wx-config --libs) $(wx-config --cxxflags) -o MyApp Dialog1.cpp Frame1.cpp
//

#include <wx/wx.h>
#include "DialogSettings.h"

// begin wxGlade: ::extracode
// end wxGlade


DialogSettings::DialogSettings(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style):
    wxDialog(parent, id, title, pos, size, wxDEFAULT_DIALOG_STYLE)
{
    // begin wxGlade: DialogSettings::DialogSettings
    wxBoxSizer* sizer_top = new wxBoxSizer(wxVERTICAL);
    notebook_settings = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0);
    sizer_top->Add(notebook_settings, 0, wxALL|wxEXPAND, 3);
    notebook_settings_pane_conn = new wxPanel(notebook_settings, wxID_ANY);
    notebook_settings->AddPage(notebook_settings_pane_conn, _("Connections"));
    wxBoxSizer* sizer_conn = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer* sizer_fastrequest = new wxStaticBoxSizer(new wxStaticBox(notebook_settings_pane_conn, wxID_ANY, _("FastRequest")), wxVERTICAL);
    sizer_conn->Add(sizer_fastrequest, 0, wxALL|wxEXPAND, 3);
    checkbox_fastrequest = new wxCheckBox(notebook_settings_pane_conn, wxID_ANY, _("Enable FastRequest"));
    sizer_fastrequest->Add(checkbox_fastrequest, 0, wxALL|wxEXPAND, 3);
    label_fastrequest = new wxStaticText(notebook_settings_pane_conn, wxID_ANY, _("Continously request updates at the specified milisecond interval:"));
    sizer_fastrequest->Add(label_fastrequest, 0, wxALL, 3);
    slider_fastrequest = new wxSlider(notebook_settings_pane_conn, wxID_ANY, 1, 1, 100, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS|wxSL_HORIZONTAL|wxSL_LABELS);
    slider_fastrequest->SetToolTip(_("Continously ask the server for updates instead of just asking after each received server message. Use this on high latency links."));
    sizer_fastrequest->Add(slider_fastrequest, 0, wxALL|wxEXPAND, 3);
    wxStaticBoxSizer* sizer_qos = new wxStaticBoxSizer(new wxStaticBox(notebook_settings_pane_conn, wxID_ANY, _("Quality of Service")), wxHORIZONTAL);
    sizer_conn->Add(sizer_qos, 0, wxALL|wxEXPAND, 3);
    checkbox_qos_ef = new wxCheckBox(notebook_settings_pane_conn, wxID_ANY, _("Enable Expedited Forwarding tagging for sent data"));
    sizer_qos->Add(checkbox_qos_ef, 0, wxALL|wxEXPAND, 3);
    wxStaticBoxSizer* sizer_multicast = new wxStaticBoxSizer(new wxStaticBox(notebook_settings_pane_conn, wxID_ANY, _("MulticastVNC")), wxVERTICAL);
    sizer_conn->Add(sizer_multicast, 0, wxALL|wxEXPAND, 3);
    checkbox_multicast = new wxCheckBox(notebook_settings_pane_conn, wxID_ANY, _("Enable MulticastVNC"));
    sizer_multicast->Add(checkbox_multicast, 0, wxALL|wxEXPAND, 3);
    label_socketrecvbuf = new wxStaticText(notebook_settings_pane_conn, wxID_ANY, _("Socket Receive Buffer Size (kB):"));
    sizer_multicast->Add(label_socketrecvbuf, 0, wxALL, 3);
    slider_socketrecvbuf = new wxSlider(notebook_settings_pane_conn, wxID_ANY, 65, 65, 9750, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_LABELS);
    slider_socketrecvbuf->SetToolTip(_("Set the multicast socket receive buffer size. Increasing the value may help against packet loss. Note that the maximum value is operating system dependent."));
    sizer_multicast->Add(slider_socketrecvbuf, 0, wxALL|wxEXPAND, 3);
    label_recvbuf = new wxStaticText(notebook_settings_pane_conn, wxID_ANY, _("Receive Buffer Size (kB):"));
    sizer_multicast->Add(label_recvbuf, 0, wxALL, 3);
    slider_recvbuf = new wxSlider(notebook_settings_pane_conn, wxID_ANY, 65, 65, 9750, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_LABELS);
    slider_recvbuf->SetToolTip(_("Set the multicast receive buffer size. Increasing the value may help against packet loss. The size of this buffer is independent of the operating system."));
    sizer_multicast->Add(slider_recvbuf, 0, wxALL|wxEXPAND, 3);
    notebook_settings_pane_encodings = new wxPanel(notebook_settings, wxID_ANY);
    notebook_settings->AddPage(notebook_settings_pane_encodings, _("Encodings"));
    wxBoxSizer* sizer_encodings = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer* sizer_encbox = new wxStaticBoxSizer(new wxStaticBox(notebook_settings_pane_encodings, wxID_ANY, _("Enabled Encodings")), wxHORIZONTAL);
    sizer_encodings->Add(sizer_encbox, 0, wxALL|wxEXPAND, 3);
    wxGridSizer* grid_sizer_encodings = new wxGridSizer(4, 3, 0, 0);
    sizer_encbox->Add(grid_sizer_encodings, 1, 0, 3);
    checkbox_enc_copyrect = new wxCheckBox(notebook_settings_pane_encodings, wxID_ANY, _("CopyRect"));
    grid_sizer_encodings->Add(checkbox_enc_copyrect, 0, wxALL, 3);
    checkbox_enc_hextile = new wxCheckBox(notebook_settings_pane_encodings, wxID_ANY, _("Hextile"));
    grid_sizer_encodings->Add(checkbox_enc_hextile, 0, wxALL, 3);
    checkbox_enc_rre = new wxCheckBox(notebook_settings_pane_encodings, wxID_ANY, _("RRE"));
    grid_sizer_encodings->Add(checkbox_enc_rre, 0, wxALL, 3);
    checkbox_enc_corre = new wxCheckBox(notebook_settings_pane_encodings, wxID_ANY, _("CoRRE"));
    grid_sizer_encodings->Add(checkbox_enc_corre, 0, wxALL, 3);
    checkbox_enc_zlib = new wxCheckBox(notebook_settings_pane_encodings, wxID_ANY, _("Zlib"));
    grid_sizer_encodings->Add(checkbox_enc_zlib, 0, wxALL, 3);
    checkbox_enc_zlibhex = new wxCheckBox(notebook_settings_pane_encodings, wxID_ANY, _("ZlibHex"));
    grid_sizer_encodings->Add(checkbox_enc_zlibhex, 0, wxALL, 3);
    checkbox_enc_zrle = new wxCheckBox(notebook_settings_pane_encodings, wxID_ANY, _("ZRLE"));
    grid_sizer_encodings->Add(checkbox_enc_zrle, 0, wxALL, 3);
    checkbox_enc_zywrle = new wxCheckBox(notebook_settings_pane_encodings, wxID_ANY, _("ZYWRLE"));
    grid_sizer_encodings->Add(checkbox_enc_zywrle, 0, wxALL, 3);
    checkbox_enc_ultra = new wxCheckBox(notebook_settings_pane_encodings, wxID_ANY, _("Ultra"));
    grid_sizer_encodings->Add(checkbox_enc_ultra, 0, wxALL, 3);
    checkbox_enc_tight = new wxCheckBox(notebook_settings_pane_encodings, wxID_ANY, _("Tight"));
    grid_sizer_encodings->Add(checkbox_enc_tight, 0, wxALL, 3);
    grid_sizer_encodings->Add(0, 0, 0, 0, 0);
    grid_sizer_encodings->Add(0, 0, 0, 0, 0);
    wxStaticBoxSizer* sizer_tightsettings = new wxStaticBoxSizer(new wxStaticBox(notebook_settings_pane_encodings, wxID_ANY, _("Lossy Encodings Settings")), wxVERTICAL);
    sizer_encodings->Add(sizer_tightsettings, 0, wxALL|wxEXPAND, 3);
    label_compresslevel = new wxStaticText(notebook_settings_pane_encodings, wxID_ANY, _("Compression level for 'Tight' and 'Zlib' encodings:"));
    sizer_tightsettings->Add(label_compresslevel, 0, wxALL, 3);
    slider_compresslevel = new wxSlider(notebook_settings_pane_encodings, wxID_ANY, 0, 0, 9, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS|wxSL_HORIZONTAL|wxSL_LABELS);
    slider_compresslevel->SetToolTip(_("Use specified compression level (0..9) for 'Tight' and 'Zlib' encodings. Level 1 uses minimum of CPU time and achieves weak compression ratios, while level 9 offers best compression but is slow in terms of CPU time consumption on the server side. Use high levels with very slow network connections, and low levels when working over high-speed LANs."));
    sizer_tightsettings->Add(slider_compresslevel, 0, wxALL|wxEXPAND, 3);
    label_quality = new wxStaticText(notebook_settings_pane_encodings, wxID_ANY, _("Quality level for 'Tight' and 'ZYWRLE' encoding:"));
    sizer_tightsettings->Add(label_quality, 0, wxALL, 3);
    slider_quality = new wxSlider(notebook_settings_pane_encodings, wxID_ANY, 0, 0, 9, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS|wxSL_HORIZONTAL|wxSL_LABELS);
    slider_quality->SetToolTip(_("Use the specified quality level (0..9) for the 'Tight' and 'ZYWRLE' encodings. Quality level 0 denotes bad image quality but very impressive compression ratios, while level 9 offers very good image quality at lower compression ratios. Note that the \"tight\" encoder uses JPEG to encode only those screen areas that look suitable for lossy compression, so quality level 0 does not always mean unacceptable image quality."));
    sizer_tightsettings->Add(slider_quality, 0, wxALL|wxEXPAND, 3);
    notebook_settings_pane_logging = new wxPanel(notebook_settings, wxID_ANY);
    notebook_settings->AddPage(notebook_settings_pane_logging, _("Logging"));
    wxBoxSizer* sizer_logging = new wxBoxSizer(wxVERTICAL);
    checkbox_logfile = new wxCheckBox(notebook_settings_pane_logging, wxID_ANY, _("Write VNC log to logfile (MultiVNC.log)"));
    sizer_logging->Add(checkbox_logfile, 0, wxALL, 6);
    checkbox_stats_save = new wxCheckBox(notebook_settings_pane_logging, wxID_ANY, _("Autosave statistics on close"));
    sizer_logging->Add(checkbox_stats_save, 0, wxALL, 6);
    
    notebook_settings_pane_logging->SetSizer(sizer_logging);
    notebook_settings_pane_encodings->SetSizer(sizer_encodings);
    notebook_settings_pane_conn->SetSizer(sizer_conn);
    SetSizer(sizer_top);
    sizer_top->Fit(this);
    Layout();
    // end wxGlade
}

