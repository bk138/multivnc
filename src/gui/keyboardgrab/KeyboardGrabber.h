#pragma once

#include <wx/window.h>

class KeyboardGrabber
{
public:
  static bool hasPermission();
  static void showPermissionDialog();

  ~KeyboardGrabber() { ungrab(); }

  void grab(wxWindow* window);
  void ungrab();
  bool isGrabbed() const { return m_grab != nullptr; }
  wxWindow* getDestinationWindow() const { return m_destinationWindow; }

private:
  void* m_grab = nullptr;
  wxWindow* m_destinationWindow = nullptr;
};
