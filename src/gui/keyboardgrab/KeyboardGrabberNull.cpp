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
  m_grabbed = true;
}


void KeyboardGrabber::ungrab()
{
  m_grabbed = false;
}
