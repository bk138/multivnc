#include <wx/wx.h>
#include <wx/menu.h>

// ...

void MultiVNC::OnShowScreenDimensions(wxCommandEvent& event) {
    wxDisplay display;
    wxSize screenSize = display.GetClientArea().GetSize();
    wxString dimensions = wxString::Format("%dx%d", screenSize.GetWidth(), screenSize.GetHeight());
    wxMessageBox(dimensions, "Available Screen Dimensions", wxOK | wxICON_INFORMATION);
}

void MultiVNC::CreateMenu() {
    // ...
    wxMenu* menu = new wxMenu;
    // ...
    menu->Append(wxID_ANY, "Show Available Screen Dimensions", "Show the available screen dimensions");
    menu->Bind(wxEVT_MENU, &MultiVNC::OnShowScreenDimensions, this);
    // ...
}