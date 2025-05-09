// -*- C++ -*-
//
// generated by wxGlade 1.0.4 on Sat Feb 22 10:50:07 2025
//
// Example for compiling a single file project under Linux using g++:
//  g++ MyApp.cpp $(wx-config --libs) $(wx-config --cxxflags) -o MyApp
//
// Example for compiling a multi file project under Linux using g++:
//  g++ main.cpp $(wx-config --libs) $(wx-config --cxxflags) -o MyApp Dialog1.cpp Frame1.cpp
//

#include <wx/wx.h>
#include "DialogNewConnection.h"

// begin wxGlade: ::extracode
// end wxGlade


DialogNewConnection::DialogNewConnection(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style):
    wxDialog(parent, id, title, pos, size, wxDEFAULT_DIALOG_STYLE)
{
    // begin wxGlade: DialogNewConnection::DialogNewConnection
    wxBoxSizer* sizer_top = new wxBoxSizer(wxVERTICAL);
    wxStaticText* label_host = new wxStaticText(this, wxID_ANY, _("Enter host to connect to:"));
    sizer_top->Add(label_host, 0, wxALL, 12);
    text_ctrl_host = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
    sizer_top->Add(text_ctrl_host, 0, wxEXPAND|wxLEFT|wxRIGHT, 18);
    panel_advanced = new wxPanel(this, wxID_ANY);
    sizer_top->Add(panel_advanced, 0, wxEXPAND, 0);
    wxBoxSizer* sizer_advanced = new wxBoxSizer(wxVERTICAL);
    wxStaticText* label_repeater_id = new wxStaticText(panel_advanced, wxID_ANY, _("If connecting to a repeater, enter the session ID here:"));
    sizer_advanced->Add(label_repeater_id, 0, wxALL, 12);
    text_ctrl_repeater_id = new wxTextCtrl(panel_advanced, wxID_ANY, wxEmptyString);
    sizer_advanced->Add(text_ctrl_repeater_id, 0, wxEXPAND|wxLEFT|wxRIGHT, 18);
    coll_pane_advanced = new wxCollapsiblePane(this, wxID_ANY, _("Advanced"));
    wxWindow *pane_win = coll_pane_advanced->GetPane();
    wxSizer *pane_sizer = new wxBoxSizer(wxVERTICAL);
    
    // previous code as set root as parent of panel_advanced, reparent
    panel_advanced->Reparent(pane_win);
    
    // previous code has attached panel_advanced there, detach and re-add to ours
    sizer_top->Detach(panel_advanced);
    pane_sizer->Add(panel_advanced, 1, wxGROW|wxALL, 0);
    
    pane_win->SetSizer(pane_sizer);
    pane_sizer->SetSizeHints(pane_win);
    sizer_top->Add(coll_pane_advanced, 0, wxALL|wxEXPAND, 12);
    wxStaticLine* static_line_1 = new wxStaticLine(this, wxID_ANY);
    sizer_top->Add(static_line_1, 0, wxEXPAND|wxLEFT|wxRIGHT, 12);
    wxBoxSizer* sizer_buttons = new wxBoxSizer(wxHORIZONTAL);
    sizer_top->Add(sizer_buttons, 0, wxALIGN_RIGHT|wxALL, 10);
    button_CANCEL = new wxButton(this, wxID_CANCEL, wxEmptyString);
    sizer_buttons->Add(button_CANCEL, 0, wxALL, 3);
    button_OK = new wxButton(this, wxID_OK, wxEmptyString);
    button_OK->SetDefault();
    sizer_buttons->Add(button_OK, 0, wxALL, 3);
    
    panel_advanced->SetSizer(sizer_advanced);
    SetSizer(sizer_top);
    sizer_top->Fit(this);
    SetAffirmativeId(button_OK->GetId());
    SetEscapeId(button_CANCEL->GetId());
    
    Layout();
    // end wxGlade
}

