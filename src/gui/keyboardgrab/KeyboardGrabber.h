#pragma once

#include <wx/window.h>

class KeyboardGrabber
{
public:
  static bool hasPermission();
  static void showPermissionDialog();

  void grab(wxWindow* window);
  void ungrab();
  bool isGrabbed() const { return m_grabbed; }

private:
  bool m_grabbed = false;
};
