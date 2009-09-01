// -*- C++ -*- generated by wxGlade 0.6.3 on Tue Sep  1 17:18:21 2009

#include "FrameMain.h"

// begin wxGlade: ::extracode

// end wxGlade


FrameMain::FrameMain(wxWindow* parent, int id, const wxString& title, const wxPoint& pos, const wxSize& size, long style):
    wxFrame(parent, id, title, pos, size, wxDEFAULT_FRAME_STYLE)
{
    // begin wxGlade: FrameMain::FrameMain
    panel_top = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSTATIC_BORDER|wxTAB_TRAVERSAL);
    splitwin_main = new wxSplitterWindow(panel_top, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_BORDER|wxSP_LIVE_UPDATE);
    splitwin_main_pane_2 = new wxPanel(splitwin_main, wxID_ANY);
    notebook_1 = new wxNotebook(splitwin_main_pane_2, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0);
    splitwin_main_pane_1 = new wxPanel(splitwin_main, wxID_ANY);
    splitwin_left = new wxSplitterWindow(splitwin_main_pane_1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_BORDER|wxSP_LIVE_UPDATE);
    splitwin_left_pane_2 = new wxPanel(splitwin_left, wxID_ANY);
    splitwin_leftlower = new wxSplitterWindow(splitwin_left_pane_2, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_BORDER|wxSP_LIVE_UPDATE);
    splitwin_leftlower_pane_2 = new wxPanel(splitwin_leftlower, wxID_ANY);
    splitwin_leftlower_pane_1 = new wxPanel(splitwin_leftlower, wxID_ANY);
    splitwin_left_pane_1 = new wxPanel(splitwin_left, wxID_ANY);
    sizer_bookmarks_staticbox = new wxStaticBox(splitwin_leftlower_pane_1, -1, _("Bookmarks"));
    sizer_3_staticbox = new wxStaticBox(splitwin_leftlower_pane_2, -1, _("Statistics"));
    sizer_services_staticbox = new wxStaticBox(splitwin_left_pane_1, -1, _("Available VNC Servers"));
    frame_main_menubar = new wxMenuBar();
    wxMenu* wxglade_tmp_menu_1 = new wxMenu();
    wxglade_tmp_menu_1->Append(wxID_YES, _("&Connect..."), _("Connect to a specific host."), wxITEM_NORMAL);
    wxglade_tmp_menu_1->Append(wxID_STOP, _("&Disconnect"), _("Terminate connection."), wxITEM_NORMAL);
    wxglade_tmp_menu_1->Append(wxID_FILE, _("Show &Log"), _("Show detailed log."), wxITEM_NORMAL);
    wxglade_tmp_menu_1->Append(wxID_PREFERENCES, wxEmptyString, _("Change preferences."), wxITEM_NORMAL);
    wxglade_tmp_menu_1->AppendSeparator();
    wxglade_tmp_menu_1->Append(wxID_EXIT, wxEmptyString, _("Exit MultiVNC."), wxITEM_NORMAL);
    frame_main_menubar->Append(wxglade_tmp_menu_1, _("&Machine"));
    wxMenu* wxglade_tmp_menu_2 = new wxMenu();
    wxglade_tmp_menu_2->Append(ID_TOOLBAR, _("Toolbar"), wxEmptyString, wxITEM_CHECK);
    wxglade_tmp_menu_2->Append(ID_DISCOVERED, _("Discovered Servers"), wxEmptyString, wxITEM_CHECK);
    wxglade_tmp_menu_2->Append(ID_BOOKMARKS, _("Bookmarks"), wxEmptyString, wxITEM_CHECK);
    wxglade_tmp_menu_2->Append(ID_STATISTICS, _("Statistics"), wxEmptyString, wxITEM_CHECK);
    wxglade_tmp_menu_2->AppendSeparator();
    wxglade_tmp_menu_2->Append(ID_FULLSCREEN, _("Fullscreen"), wxEmptyString, wxITEM_CHECK);
    frame_main_menubar->Append(wxglade_tmp_menu_2, _("&View"));
    wxMenu* wxglade_tmp_menu_3 = new wxMenu();
    wxglade_tmp_menu_3->Append(wxID_ADD, _("&Add Bookmark"), wxEmptyString, wxITEM_NORMAL);
    frame_main_menubar->Append(wxglade_tmp_menu_3, _("&Bookmarks"));
    wxMenu* wxglade_tmp_menu_4 = new wxMenu();
    wxglade_tmp_menu_4->Append(wxID_HELP, _("&Contents"), _("Show Help."), wxITEM_NORMAL);
    wxglade_tmp_menu_4->Append(wxID_ABOUT, wxEmptyString, wxEmptyString, wxITEM_NORMAL);
    frame_main_menubar->Append(wxglade_tmp_menu_4, _("&Help"));
    SetMenuBar(frame_main_menubar);
    frame_main_statusbar = CreateStatusBar(2, 0);
    frame_main_toolbar = new wxToolBar(this, -1, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL|wxTB_DOCKABLE|wxTB_3DBUTTONS|wxTB_TEXT);
    SetToolBar(frame_main_toolbar);
    frame_main_toolbar->SetToolBitmapSize(wxSize(24, 24));
    frame_main_toolbar->AddTool(wxID_YES, _("Connect"), (bitmapFromMem(connect_png)), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    frame_main_toolbar->AddTool(wxID_CANCEL, _("Disconnect"), (bitmapFromMem(disconnect_png)), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    frame_main_toolbar->AddSeparator();
    frame_main_toolbar->AddTool(ID_FULLSCREEN, _("Fullscreen"), (bitmapFromMem(fullscreen_png)), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    frame_main_toolbar->AddTool(wxID_SAVE, _("Take Screenshot"), (bitmapFromMem(screenshot_png)), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    frame_main_toolbar->Realize();
    const wxString *list_box_services_choices = NULL;
    list_box_services = new wxListBox(splitwin_left_pane_1, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, list_box_services_choices, wxLB_SINGLE|wxLB_HSCROLL|wxLB_NEEDED_SB);
    const wxString *list_box_bookmarks_choices = NULL;
    list_box_bookmarks = new wxListBox(splitwin_leftlower_pane_1, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, list_box_bookmarks_choices, wxLB_SINGLE|wxLB_HSCROLL|wxLB_NEEDED_SB);
    label_1 = new wxStaticText(splitwin_leftlower_pane_2, wxID_ANY, _("FPS:"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE);
    text_ctrl_1 = new wxTextCtrl(splitwin_leftlower_pane_2, wxID_ANY, _("123"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    label_2 = new wxStaticText(splitwin_leftlower_pane_2, wxID_ANY, _("Lat:"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE);
    text_ctrl_2 = new wxTextCtrl(splitwin_leftlower_pane_2, wxID_ANY, _("456"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    notebook_1_pane_1 = new wxPanel(notebook_1, wxID_ANY);

    set_properties();
    do_layout();
    // end wxGlade
}


BEGIN_EVENT_TABLE(FrameMain, wxFrame)
    // begin wxGlade: FrameMain::event_table
    EVT_MENU(wxID_YES, FrameMain::machine_connect)
    EVT_MENU(wxID_STOP, FrameMain::machine_disconnect)
    EVT_MENU(wxID_FILE, FrameMain::machine_showlog)
    EVT_MENU(wxID_PREFERENCES, FrameMain::machine_preferences)
    EVT_MENU(wxID_EXIT, FrameMain::machine_exit)
    EVT_MENU(ID_TOOLBAR, FrameMain::view_toggletoolbar)
    EVT_MENU(ID_DISCOVERED, FrameMain::view_togglediscovered)
    EVT_MENU(ID_BOOKMARKS, FrameMain::view_togglebookmarks)
    EVT_MENU(ID_STATISTICS, FrameMain::view_togglestatistics)
    EVT_MENU(ID_FULLSCREEN, FrameMain::view_togglefullscreen)
    EVT_MENU(wxID_ADD, FrameMain::bookmarks_add)
    EVT_MENU(wxID_HELP, FrameMain::help_contents)
    EVT_MENU(wxID_ABOUT, FrameMain::help_about)
    EVT_TOOL(wxID_CANCEL, FrameMain::machine_disconnect)
    EVT_TOOL(ID_FULLSCREEN, FrameMain::view_togglefullscreen)
    EVT_TOOL(wxID_SAVE, FrameMain::machine_screenshot)
    EVT_LISTBOX_DCLICK(wxID_ANY, FrameMain::listbox_services_dclick)
    EVT_LISTBOX(wxID_ANY, FrameMain::listbox_services_select)
    EVT_LISTBOX_DCLICK(wxID_ANY, FrameMain::listbox_bookmarks_dclick)
    EVT_LISTBOX(wxID_ANY, FrameMain::listbox_bookmarks_select)
    // end wxGlade
END_EVENT_TABLE();


void FrameMain::machine_connect(wxCommandEvent &event)
{
    event.Skip();
    wxLogDebug(wxT("Event handler (FrameMain::machine_connect) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void FrameMain::machine_disconnect(wxCommandEvent &event)
{
    event.Skip();
    wxLogDebug(wxT("Event handler (FrameMain::machine_disconnect) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void FrameMain::machine_showlog(wxCommandEvent &event)
{
    event.Skip();
    wxLogDebug(wxT("Event handler (FrameMain::machine_showlog) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void FrameMain::machine_preferences(wxCommandEvent &event)
{
    event.Skip();
    wxLogDebug(wxT("Event handler (FrameMain::machine_preferences) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void FrameMain::machine_exit(wxCommandEvent &event)
{
    event.Skip();
    wxLogDebug(wxT("Event handler (FrameMain::machine_exit) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void FrameMain::view_toggletoolbar(wxCommandEvent &event)
{
    event.Skip();
    wxLogDebug(wxT("Event handler (FrameMain::view_toggletoolbar) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void FrameMain::view_togglediscovered(wxCommandEvent &event)
{
    event.Skip();
    wxLogDebug(wxT("Event handler (FrameMain::view_togglediscovered) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void FrameMain::view_togglebookmarks(wxCommandEvent &event)
{
    event.Skip();
    wxLogDebug(wxT("Event handler (FrameMain::view_togglebookmarks) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void FrameMain::view_togglestatistics(wxCommandEvent &event)
{
    event.Skip();
    wxLogDebug(wxT("Event handler (FrameMain::view_togglestatistics) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void FrameMain::view_togglefullscreen(wxCommandEvent &event)
{
    event.Skip();
    wxLogDebug(wxT("Event handler (FrameMain::view_togglefullscreen) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void FrameMain::bookmarks_add(wxCommandEvent &event)
{
    event.Skip();
    wxLogDebug(wxT("Event handler (FrameMain::bookmarks_add) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void FrameMain::help_contents(wxCommandEvent &event)
{
    event.Skip();
    wxLogDebug(wxT("Event handler (FrameMain::help_contents) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void FrameMain::help_about(wxCommandEvent &event)
{
    event.Skip();
    wxLogDebug(wxT("Event handler (FrameMain::help_about) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void FrameMain::machine_screenshot(wxCommandEvent &event)
{
    event.Skip();
    wxLogDebug(wxT("Event handler (FrameMain::machine_screenshot) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void FrameMain::listbox_services_dclick(wxCommandEvent &event)
{
    event.Skip();
    wxLogDebug(wxT("Event handler (FrameMain::listbox_services_dclick) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void FrameMain::listbox_services_select(wxCommandEvent &event)
{
    event.Skip();
    wxLogDebug(wxT("Event handler (FrameMain::listbox_services_select) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void FrameMain::listbox_bookmarks_dclick(wxCommandEvent &event)
{
    event.Skip();
    wxLogDebug(wxT("Event handler (FrameMain::listbox_bookmarks_dclick) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void FrameMain::listbox_bookmarks_select(wxCommandEvent &event)
{
    event.Skip();
    wxLogDebug(wxT("Event handler (FrameMain::listbox_bookmarks_select) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


// wxGlade: add FrameMain event handlers


void FrameMain::set_properties()
{
    // begin wxGlade: FrameMain::set_properties
    SetTitle(_("MultiVNC"));
    wxIcon _icon;
    _icon.CopyFromBitmap(wxICON(icon));
    SetIcon(_icon);
    int frame_main_statusbar_widths[] = { -1, 3 };
    frame_main_statusbar->SetStatusWidths(2, frame_main_statusbar_widths);
    const wxString frame_main_statusbar_fields[] = {
        _("Status"),
        _("zwei")
    };
    for(int i = 0; i < frame_main_statusbar->GetFieldsCount(); ++i) {
        frame_main_statusbar->SetStatusText(frame_main_statusbar_fields[i], i);
    }
    list_box_bookmarks->SetForegroundColour(wxColour(0, 0, 0));
    splitwin_main->SetMinSize(wxSize(800, 540));
    // end wxGlade
}


void FrameMain::do_layout()
{
    // begin wxGlade: FrameMain::do_layout
    wxBoxSizer* sizer_top = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizer_splitwinmain = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizer_1 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizer_2 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizer_leftlower = new wxBoxSizer(wxHORIZONTAL);
    wxStaticBoxSizer* sizer_3 = new wxStaticBoxSizer(sizer_3_staticbox, wxVERTICAL);
    wxBoxSizer* sizer_5 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizer_4 = new wxBoxSizer(wxHORIZONTAL);
    wxStaticBoxSizer* sizer_bookmarks = new wxStaticBoxSizer(sizer_bookmarks_staticbox, wxHORIZONTAL);
    wxStaticBoxSizer* sizer_services = new wxStaticBoxSizer(sizer_services_staticbox, wxHORIZONTAL);
    sizer_services->Add(list_box_services, 1, wxALL|wxEXPAND, 3);
    splitwin_left_pane_1->SetSizer(sizer_services);
    sizer_bookmarks->Add(list_box_bookmarks, 1, wxALL|wxEXPAND, 3);
    splitwin_leftlower_pane_1->SetSizer(sizer_bookmarks);
    sizer_4->Add(label_1, 5, wxALL|wxEXPAND, 3);
    sizer_4->Add(text_ctrl_1, 0, wxALL|wxADJUST_MINSIZE, 3);
    sizer_3->Add(sizer_4, 1, wxEXPAND, 0);
    sizer_5->Add(label_2, 5, wxALL, 3);
    sizer_5->Add(text_ctrl_2, 0, wxALL|wxADJUST_MINSIZE, 3);
    sizer_3->Add(sizer_5, 1, wxEXPAND, 0);
    splitwin_leftlower_pane_2->SetSizer(sizer_3);
    splitwin_leftlower->SplitHorizontally(splitwin_leftlower_pane_1, splitwin_leftlower_pane_2);
    sizer_leftlower->Add(splitwin_leftlower, 1, wxALL|wxEXPAND, 3);
    splitwin_left_pane_2->SetSizer(sizer_leftlower);
    splitwin_left->SplitHorizontally(splitwin_left_pane_1, splitwin_left_pane_2);
    sizer_2->Add(splitwin_left, 1, wxALL|wxEXPAND, 3);
    splitwin_main_pane_1->SetSizer(sizer_2);
    notebook_1->AddPage(notebook_1_pane_1, _("tab1"));
    sizer_1->Add(notebook_1, 1, wxALL|wxEXPAND, 3);
    splitwin_main_pane_2->SetSizer(sizer_1);
    splitwin_main->SplitVertically(splitwin_main_pane_1, splitwin_main_pane_2, 71);
    sizer_splitwinmain->Add(splitwin_main, 1, wxALL|wxEXPAND, 3);
    panel_top->SetSizer(sizer_splitwinmain);
    sizer_top->Add(panel_top, 1, wxEXPAND, 0);
    SetSizer(sizer_top);
    sizer_top->Fit(this);
    Layout();
    // end wxGlade
}

