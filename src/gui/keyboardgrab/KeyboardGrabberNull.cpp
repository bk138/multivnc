#include "KeyboardGrabber.h"


void KeyboardGrabber::grab(wxWindow*)
{
  m_grabbed = true;
}


void KeyboardGrabber::ungrab()
{
  m_grabbed = false;
}
