#pragma once

#include <wx/window.h>

class KeyboardGrabber
{
public:
  void grab(wxWindow* window);
  void ungrab();
  bool isGrabbed() const { return m_grabbed; }

private:
  bool m_grabbed = false;
};
