#include "KeyboardGrabber.h"


bool KeyboardGrabber::hasPermission()
{
  return true;
}


void KeyboardGrabber::showPermissionDialog()
{
}


void KeyboardGrabber::grab(wxWindow*)
{
  m_grab = (void*)0x1; // Grab active
}


void KeyboardGrabber::ungrab()
{
  m_grab = nullptr;
}
