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

#ifndef FRAMELOG_H
#define FRAMELOG_H

#include <wx/wx.h>
#include <wx/image.h>
#include <wx/intl.h>

#ifndef APP_CATALOG
#define APP_CATALOG "app"  // replace with the appropriate catalog name
#endif


// begin wxGlade: ::dependencies
// end wxGlade

// begin wxGlade: ::extracode
// end wxGlade


class FrameLog: public wxFrame {
public:
    // begin wxGlade: FrameLog::ids
    // end wxGlade

    FrameLog(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos=wxDefaultPosition, const wxSize& size=wxDefaultSize, long style=wxDEFAULT_FRAME_STYLE);

private:

protected:
    // begin wxGlade: FrameLog::attributes
    wxPanel* panel_top;
    wxTextCtrl* text_ctrl_log;
    wxButton* button_clear;
    wxButton* button_save;
    wxButton* button_close;
    // end wxGlade

    DECLARE_EVENT_TABLE();

public:
    virtual void log_clear(wxCommandEvent &event); // wxGlade: <event_handler>
    virtual void log_saveas(wxCommandEvent &event); // wxGlade: <event_handler>
    virtual void log_close(wxCommandEvent &event); // wxGlade: <event_handler>
}; // wxGlade: end class


#endif // FRAMELOG_H
