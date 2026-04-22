#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <wx/log.h>
#include "KeyboardGrabber.h"


bool KeyboardGrabber::hasPermission()
{
  return true;
}


void KeyboardGrabber::showPermissionDialog()
{
}


void KeyboardGrabber::grab(wxWindow* window)
{
  if(m_grab)
    return;

  GdkGrabStatus status = gdk_keyboard_grab(gtk_widget_get_window(window->GetHandle()), True, GDK_CURRENT_TIME);
  if(status == GDK_GRAB_SUCCESS) {
      m_grab = (void*)0x1; // Grab active
      wxLogDebug("KeyboardGrabber::grab: successful");
  } else {
      wxLogDebug("KeyboardGrabber::grab: failed with status %d", status);
  }
}


void KeyboardGrabber::ungrab()
{
  gdk_keyboard_ungrab(GDK_CURRENT_TIME);

  m_grab = nullptr;
  wxLogDebug("KeyboardGrabber::ungrab");
}
