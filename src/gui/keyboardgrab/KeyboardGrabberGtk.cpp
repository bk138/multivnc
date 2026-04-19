#define GSocket GlibGSocket
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#undef GSocket

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

  gdk_keyboard_grab(gtk_widget_get_window(window->GetHandle()), True, GDK_CURRENT_TIME);

  m_grab = (void*)0x1; // Grab active
  wxLogDebug("%s", __func__);
}


void KeyboardGrabber::ungrab()
{
  gdk_keyboard_ungrab(GDK_CURRENT_TIME);

  m_grab = nullptr;
  wxLogDebug("%s", __func__);
}
