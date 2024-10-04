

#include <wx/gdicmn.h>
#include <wx/log.h>
#include <wx/math.h>
#include <wx/sizer.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/dcclient.h>
#include <wx/tokenzr.h>
#ifdef __WXGTK__
#define GSocket GlibGSocket
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#undef GSocket
#endif
#include "res/vnccursor.xbm"
#include "res/vnccursor-mask.xbm"
#include "MultiVNCApp.h"
#include "ViewerWindow.h"



/********************************************

  VNCCanvas class

********************************************/


/*
  canvas that displays a VNCConn's framebuffer
  and submits mouse and key events.

*/
class VNCCanvas: public wxPanel
{
  bool saved_enable_mnemonics;
  bool saved_enable_accels;
  char *saved_menubar_accel;
  bool keyboard_grabbed;

  wxTimer update_timer;
  void onUpdateTimer(wxTimerEvent& event);
  void onPaint(wxPaintEvent &event);
  void onMouseAction(wxMouseEvent &event);
  void onKeyDown(wxKeyEvent &event);
  void onKeyUp(wxKeyEvent &event);
  void onChar(wxKeyEvent &event);
  void onFocusGain(wxFocusEvent &event);
  void onFocusLoss(wxFocusEvent &event);

protected:
  DECLARE_EVENT_TABLE();

public:
  /// Creates a new canvas with 0,0 size. Need to call adjustCanvasSize()!
  VNCCanvas(wxWindow* parent, VNCConn* c);
  void grab_keyboard();
  void ungrab_keyboard();

  VNCConn* conn;
  wxRegion updated_area;
  double scale_factor = 1.0;
  bool do_keyboard_grab;
};
	


#define VNCCANVAS_UPDATE_TIMER_ID 1
#define VNCCANVAS_UPDATE_TIMER_INTERVAL 30

BEGIN_EVENT_TABLE(VNCCanvas, wxPanel)
    EVT_PAINT  (VNCCanvas::onPaint)
    EVT_MOUSE_EVENTS (VNCCanvas::onMouseAction)
    EVT_KEY_DOWN (VNCCanvas::onKeyDown)
    EVT_KEY_UP (VNCCanvas::onKeyUp)
    EVT_CHAR (VNCCanvas::onChar)
    EVT_SET_FOCUS (VNCCanvas::onFocusGain)
    EVT_KILL_FOCUS(VNCCanvas::onFocusLoss)
    EVT_TIMER (VNCCANVAS_UPDATE_TIMER_ID, VNCCanvas::onUpdateTimer)
END_EVENT_TABLE();



/*
  constructor/destructor 
  (make sure size is set to 0,0 or win32 gets stuck sending 
  paint events in listen mode)
*/

VNCCanvas::VNCCanvas(wxWindow* parent, VNCConn* c):
  wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(0,0), wxWANTS_CHARS)
{
  conn = c;

  keyboard_grabbed = do_keyboard_grab = false;
 
  // this kinda cursor creation works everywhere
  wxBitmap vnccursor_bitmap((char*)vnccursor_bits, 16, 16);
  wxBitmap vnccursor_mask_bitmap((char*)vnccursor_mask_bits, 16, 16);
  vnccursor_bitmap.SetMask(new wxMask(vnccursor_mask_bitmap));
  wxImage vnccursor_image = vnccursor_bitmap.ConvertToImage();
  vnccursor_image.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, 8);
  vnccursor_image.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, 8);
  SetCursor(wxCursor(vnccursor_image));

  update_timer.SetOwner(this, VNCCANVAS_UPDATE_TIMER_ID);
  update_timer.Start(VNCCANVAS_UPDATE_TIMER_INTERVAL);

#ifndef NDEBUG
  SetBackgroundColour(*wxRED);
#endif
}



/*
  private members
*/


void VNCCanvas::grab_keyboard()
{
  if(!keyboard_grabbed)
    {
#ifdef __WXGTK__
      // grab
      gdk_keyboard_grab(gtk_widget_get_window(GetHandle()), True, GDK_CURRENT_TIME);

      // save previous settings
      GtkSettings *settings = gtk_settings_get_for_screen(gdk_screen_get_default());
      g_object_get(settings, "gtk-enable-mnemonics", &saved_enable_mnemonics, NULL);
      g_object_get(settings, "gtk-enable-accels", &saved_enable_accels, NULL);
      g_object_get(settings, "gtk-menu-bar-accel", &saved_menubar_accel, NULL);
 
      // and disable keyboard shortcuts
      g_object_set(settings, "gtk-enable-mnemonics", false, NULL);
      g_object_set(settings, "gtk-enable-accels", false, NULL);
      g_object_set(settings, "gtk-menu-bar-accel", NULL, NULL);
#endif

      keyboard_grabbed = true;
      wxLogDebug(wxT("VNCCanvas %p: grabbed keyboard"), this);
    }
}



void VNCCanvas::ungrab_keyboard()
{
#ifdef __WXGTK__
  // ungrab
  gdk_keyboard_ungrab(GDK_CURRENT_TIME);
  // restore to saved values
  GtkSettings *settings = gtk_settings_get_for_screen(gdk_screen_get_default());
  g_object_set(settings, "gtk-enable-mnemonics", saved_enable_mnemonics, NULL);
  g_object_set(settings, "gtk-enable-accels", saved_enable_accels, NULL);
  g_object_set(settings, "gtk-menu-bar-accel", saved_menubar_accel, NULL);

#endif

  keyboard_grabbed = false;
  wxLogDebug(wxT("VNCCanvas %p: ungrabbed keyboard"), this);
}



void VNCCanvas::onUpdateTimer(wxTimerEvent& event)
{
  // get the update rect list
  wxRegionIterator upd(updated_area); 
  while(upd)
    {
      wxRect update_rect(upd.GetRect());

      if(scale_factor != 1.0) {
          update_rect.x *= scale_factor;
          update_rect.y *= scale_factor;
          update_rect.width *= scale_factor;
          update_rect.height *= scale_factor;

          // fixes artifacts. +2 because double->int cuttofs can happen for x,y _and_ w,h scaling
          update_rect.width += 2;
          update_rect.height += 2;
      }

      wxLogDebug(wxT("VNCCanvas %p: invalidating updated rect: (%i,%i,%i,%i)"),
		 this,
		 update_rect.x,
		 update_rect.y,
		 update_rect.width,
		 update_rect.height);

      // triggers onPaint()
      Refresh(false, &update_rect);
      ++upd;
    }

  updated_area.Clear();

}


void VNCCanvas::onPaint(wxPaintEvent &WXUNUSED(event))
{
#ifndef NDEBUG
  wxLongLong t0 = wxGetLocalTimeMillis();
  size_t nr_rects = 0;
  size_t nr_pixels = 0;
#endif

  // this happens on GTK even if our size is (0,0)
  if(GetSize().GetWidth() == 0 || GetSize().GetHeight() == 0)
    return;

  wxPaintDC dc(this);
  dc.SetUserScale(scale_factor, scale_factor);

  // get the update rect list
  wxRegionIterator upd(GetUpdateRegion()); 
  while(upd)
    {
      wxRect update_rect(upd.GetRect());

      if(scale_factor != 1.0) {
          update_rect.x /= scale_factor;
          update_rect.y /= scale_factor;
          update_rect.width /= scale_factor;
          update_rect.height /= scale_factor;

          // fixes artifacts. +2 because double->int cuttofs can happen for x,y _and_ w,h scaling
          update_rect.width += 2;
          update_rect.height += 2;

          // make sure this is always within the framebuffer boudaries;
          // might not always be due to the artifact fix above, would not be drawn then
          update_rect.Intersect(wxRect(0,
                                       0,
                                       conn->getFrameBufferWidth(),
                                       conn->getFrameBufferHeight()));
      }

      wxLogDebug(wxT("VNCCanvas %p: got repaint event: (%i,%i,%i,%i)"),
		 this,
		 update_rect.x,
		 update_rect.y,
		 update_rect.width,
		 update_rect.height);

#ifndef NDEBUG
      ++nr_rects;
      nr_pixels += update_rect.width * update_rect.height;
#endif
    
      const wxBitmap& region = conn->getFrameBufferRegion(update_rect);
      if(region.IsOk())
	dc.DrawBitmap(region, update_rect.x, update_rect.y);
	
      ++upd;
    }

#ifndef NDEBUG
  wxLongLong t1 = wxGetLocalTimeMillis();
  wxLogDebug(wxT("VNCCanvas %p: updating %zu rects (%f megapixels) took %lld ms"),
	     this,
	     nr_rects,
	     (double)nr_pixels/(1024.0*1024.0),
	     (t1-t0).GetValue());
#endif
}






void VNCCanvas::onMouseAction(wxMouseEvent &event)
{
  if(event.Entering())
    {
      SetFocus();

      if(do_keyboard_grab)
	grab_keyboard();

      if(wxTheClipboard->Open()) 
	{
	  if(wxTheClipboard->IsSupported(wxDF_TEXT))
	    {
	      wxTextDataObject data;
	      wxTheClipboard->GetData(data);
	      wxString text = data.GetText();
	      wxLogDebug(wxT("VNCCanvas %p: setting cuttext: '%s'"), this, text.c_str());
	      conn->setCuttext(text);
	    }
	  wxTheClipboard->Close();
	}
    }

  if(event.Leaving())
    {
      if(do_keyboard_grab)
	ungrab_keyboard();
    }

  // use rounding here to be a bit more correct if scaling causes fractional values
  event.m_x = std::round(event.m_x / scale_factor);
  event.m_y = std::round(event.m_y / scale_factor);

  conn->sendPointerEvent(event);
}


void VNCCanvas::onKeyDown(wxKeyEvent &event)
{
  conn->sendKeyEvent(event, true, false);
}


void VNCCanvas::onKeyUp(wxKeyEvent &event)
{
  conn->sendKeyEvent(event, false, false);
}


void VNCCanvas::onChar(wxKeyEvent &event)
{
  conn->sendKeyEvent(event, true, true);
}


void VNCCanvas::onFocusGain(wxFocusEvent &event)
{
  wxLogDebug(wxT("VNCCanvas %p: got focus"), this);
}


void VNCCanvas::onFocusLoss(wxFocusEvent &event)
{
  wxLogDebug(wxT("VNCCanvas %p: lost focus, upping key modifiers"), this);
  
  wxKeyEvent key_event;

  key_event.m_keyCode = WXK_SHIFT;
  conn->sendKeyEvent(key_event, false, false);
  key_event.m_keyCode = WXK_ALT;
  conn->sendKeyEvent(key_event, false, false);
  key_event.m_keyCode = WXK_CONTROL;
  conn->sendKeyEvent(key_event, false, false);
}




/*
  public members
*/






/********************************************

  ViewerWindow class

********************************************/

#define VIEWERWINDOW_SCROLL_RATE 10
#define VIEWERWINDOW_STATS_TIMER_INTERVAL 100
#define VIEWERWINDOW_STATS_TIMER_ID 0

// map recv of custom events to handler methods
BEGIN_EVENT_TABLE(ViewerWindow, wxPanel)
  EVT_TIMER   (VIEWERWINDOW_STATS_TIMER_ID, ViewerWindow::onStatsTimer)
  EVT_VNCCONNUPDATENOTIFY (wxID_ANY, ViewerWindow::onVNCConnUpdateNotify)
  EVT_SIZE (ViewerWindow::onResize)
END_EVENT_TABLE()


/*
  constructor/destructor
 */

ViewerWindow::ViewerWindow(wxWindow* parent, VNCConn* conn):
  wxPanel(parent)
{
  /*
    setup main window
  */
  // a sizer dividing the window vertically
  SetSizer(new wxBoxSizer(wxVERTICAL));

  // the upper subwindow
  canvas_container = new wxScrolledWindow(this);
#ifndef NDEBUG
  canvas_container->SetBackgroundColour(*wxBLUE);
#endif

  canvas_container->SetScrollRate(VIEWERWINDOW_SCROLL_RATE, VIEWERWINDOW_SCROLL_RATE);
  GetSizer()->Add(canvas_container, 1, wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);


  // the lower subwindow
  stats_container = new wxScrolledWindow(this);
  stats_container->SetScrollRate(VIEWERWINDOW_SCROLL_RATE, VIEWERWINDOW_SCROLL_RATE);
  GetSizer()->Add(stats_container, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALL, 3);



  /*
    setup upper window
  */
  // set some default for now
  this->show_1to1  = false;
  canvas_container->SetSizer(new wxBoxSizer(wxHORIZONTAL));
  canvas = new VNCCanvas(canvas_container, conn);
  adjustCanvasSize();
  wxBoxSizer* sizer_vert_canvas = new wxBoxSizer(wxVERTICAL);
  sizer_vert_canvas->Add(canvas, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL);
  canvas_container->GetSizer()->Add(sizer_vert_canvas, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL);

  /*
    setup lower window
  */ 
  // fill lower subwindow with a vertical sizer with a horizontal static box sizer in it so all is nicely centered
  wxBoxSizer* sizer_vert = new wxBoxSizer(wxVERTICAL);
  stats_container->SetSizer(sizer_vert);
  wxStaticBoxSizer* sizer_stats = new wxStaticBoxSizer(new wxStaticBox(stats_container, -1, _("Statistics")), wxHORIZONTAL);
  sizer_vert->Add(sizer_stats, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL);

 
  // create statitistics widgets
  label_updrawbytes = new wxStaticText(stats_container, wxID_ANY, _("Eff. KB/s:"));
  text_ctrl_updrawbytes = new wxTextCtrl(stats_container, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80,20), wxTE_READONLY);
  label_updcount = new wxStaticText(stats_container, wxID_ANY, _("Updates/s:"));
  text_ctrl_updcount = new wxTextCtrl(stats_container, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80,20), wxTE_READONLY);
  label_latency = new wxStaticText(stats_container, wxID_ANY, _("Latency ms:"));
  text_ctrl_latency = new wxTextCtrl(stats_container, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80,20), wxTE_READONLY);
  label_lossratio = new wxStaticText(stats_container, wxID_ANY, _("Loss Ratio:"));
  text_ctrl_lossratio = new wxTextCtrl(stats_container, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80,20), wxTE_READONLY);
  label_recvbuf = new wxStaticText(stats_container, wxID_ANY, _("Rcv Buffer:"));
  gauge_recvbuf = new wxGauge(stats_container, wxID_ANY, 10, wxDefaultPosition, wxSize(80,20), wxGA_HORIZONTAL|wxGA_SMOOTH);

  dflt_fg = gauge_recvbuf->GetForegroundColour();

  // create grid sizers, two cause we wanna hide the multicast one sometimes
  wxGridSizer* grid_sizer_stats_uni = new wxGridSizer(2, 3, 0, 0);
  wxGridSizer* grid_sizer_stats_multi = new wxGridSizer(2, 2, 0, 0);
  // insert widgets into grid sizers
  grid_sizer_stats_uni->Add(label_updrawbytes, 0, wxALL, 3);
  grid_sizer_stats_uni->Add(label_updcount, 1, wxALL, 3);
  grid_sizer_stats_uni->Add(label_latency, 0, wxALL, 3);
  grid_sizer_stats_uni->Add(text_ctrl_updrawbytes, 0, wxLEFT|wxRIGHT|wxBOTTOM, 3);
  grid_sizer_stats_uni->Add(text_ctrl_updcount, 0, wxLEFT|wxRIGHT|wxBOTTOM, 3);
  grid_sizer_stats_uni->Add(text_ctrl_latency, 0, wxLEFT|wxRIGHT|wxBOTTOM, 3);
  grid_sizer_stats_multi->Add(label_lossratio, 0, wxALL, 3);
  grid_sizer_stats_multi->Add(label_recvbuf, 0, wxALL, 3);
  grid_sizer_stats_multi->Add(text_ctrl_lossratio, 0, wxLEFT|wxRIGHT|wxBOTTOM, 3);
  grid_sizer_stats_multi->Add(gauge_recvbuf, 0, wxLEFT|wxRIGHT|wxBOTTOM, 3);
  // insert grid sizer into static box sizer
  sizer_stats->Add(grid_sizer_stats_uni, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_BOTTOM|wxALL, 0);
  sizer_stats->Add(grid_sizer_stats_multi, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_BOTTOM|wxALL, 0);

  // IMPORTANT: make sizer obey to size hints!
  stats_container->GetSizer()->SetSizeHints(stats_container);

  stats_timer.SetOwner(this, VIEWERWINDOW_STATS_TIMER_ID);

#ifndef NDEBUG
  SetBackgroundColour(*wxGREEN);
#endif
}




ViewerWindow::~ViewerWindow()
{
  if(canvas)
    delete canvas;
}



/*
  private members
*/


void ViewerWindow::onVNCConnUpdateNotify(VNCConnUpdateNotifyEvent& event)
{
  VNCConn* sending_conn = static_cast<VNCConn*>(event.GetEventObject());

  // only do something if this is our VNCConn
  if(sending_conn == canvas->conn)
    canvas->updated_area.Union(event.rect);
}


void ViewerWindow::onStatsTimer(wxTimerEvent& event)
{
  if(canvas && canvas->conn)
    {
      VNCConn* c = canvas->conn;

      text_ctrl_updrawbytes->Clear();
      text_ctrl_updcount->Clear();
      text_ctrl_latency->Clear();
      text_ctrl_lossratio->Clear();

      if(!c->isMulticast())
	{
	  label_lossratio->Show(false);
	  text_ctrl_lossratio->Show(false);
	  label_recvbuf->Show(false);
	  gauge_recvbuf->Show(false);
	}
      else
	{
	  label_lossratio->Show(true);
	  text_ctrl_lossratio->Show(true);
	  label_recvbuf->Show(true);
	  gauge_recvbuf->Show(true);
	}
      stats_container->Layout();

      if( ! c->getStats().IsEmpty() )
	{
	  // it is imperative here to obey the format of the sample string!
	  wxStringTokenizer tokenizer(c->getStats().Last(),  wxT(","));
	  tokenizer.GetNextToken(); // skip UTC time
	  tokenizer.GetNextToken(); // skip conn time
	  tokenizer.GetNextToken(); // skip rcvd bytes
	  *text_ctrl_updrawbytes << wxAtoi(tokenizer.GetNextToken())/1024; // inflated bytes
	  *text_ctrl_updcount << tokenizer.GetNextToken();
	  int latency =  wxAtoi(tokenizer.GetNextToken());
	  if(latency >= 0)
	    *text_ctrl_latency << latency;
	 
	  double lossratio = c->getMCLossRatio();
	  if(lossratio >= 0) // can be -1 if nothing to measure
	    *text_ctrl_lossratio << lossratio;
	}

      gauge_recvbuf->SetRange(c->getMCBufSize());
      gauge_recvbuf->SetValue(c->getMCBufFill());

      // flash red when buffer full
      if(gauge_recvbuf->GetRange() == gauge_recvbuf->GetValue())
	label_recvbuf->SetForegroundColour(*wxRED);
      else
	label_recvbuf->SetForegroundColour(dflt_fg);
    }
}

void ViewerWindow::onResize(wxSizeEvent &event)
{
    wxLogDebug("ViewerWindow %p: resized to (%i, %i)", this, GetSize().GetWidth(), GetSize().GetHeight());
    adjustCanvasSize();
}

/*
  public members
*/


void ViewerWindow::adjustCanvasSize()
{
  if(canvas)
    {
        if(show_1to1) {
            // reset scale factor
            canvas->scale_factor = 1.0;
            // and enable scroll bars
            canvas_container->SetScrollRate(VIEWERWINDOW_SCROLL_RATE, VIEWERWINDOW_SCROLL_RATE);
        } else {
            /*
              calculate scale factor
             */
            wxLogDebug("ViewerWindow %p: adjustCanvasSize: framebuffer is %d x %d",
                       this,
                       canvas->conn->getFrameBufferWidth(),
                       canvas->conn->getFrameBufferHeight());
            wxLogDebug("ViewerWindow %p: adjustCanvasSize: window      is %d x %d",
                       this,
                       GetSize().GetWidth(),
                       GetSize().GetHeight());

            float width_factor = GetSize().GetWidth() / (float)canvas->conn->getFrameBufferWidth();

            // stats shown?
            int stats_height = 0;
            if(GetSizer()->IsShown(1)) {
                // compute correct size for initial case
                Layout();
                stats_height = stats_container->GetSize().GetHeight();
            }
            float height_factor = (GetSize().GetHeight() - stats_height) / (float)canvas->conn->getFrameBufferHeight();

            wxLogDebug("ViewerWindow %p: adjustCanvasSize: width factor is %f", this, width_factor);
            wxLogDebug("ViewerWindow %p: adjustCanvasSize: height factor is %f", this, height_factor);

            canvas->scale_factor = wxMin(width_factor, height_factor);

            /*
              and disable scroll bars
             */
            canvas_container->SetScrollRate(0, 0);
        }
        wxSize dimensions(canvas->conn->getFrameBufferWidth() * canvas->scale_factor,
                          canvas->conn->getFrameBufferHeight() * canvas->scale_factor);

        wxLogDebug(wxT("ViewerWindow %p: adjustCanvasSize: adjusting canvas size to (%i, %i)"),
                   this,
                   dimensions.GetWidth(),
                   dimensions.GetHeight());

        // SetSize() isn't enough...
        canvas->SetInitialSize(dimensions);
        canvas->CentreOnParent();
        Layout();
    }
}




void ViewerWindow::showStats(bool show_stats)
{
  if(show_stats)
    {
      stats_timer.Start(VIEWERWINDOW_STATS_TIMER_INTERVAL);
      GetSizer()->Show(1, true);
    }
  else
    {
      stats_timer.Stop();
      GetSizer()->Show(1, false);
    }
  adjustCanvasSize();
  Layout();

  text_ctrl_updrawbytes->Clear();
  text_ctrl_updcount->Clear();
  text_ctrl_latency->Clear();
  text_ctrl_lossratio->Clear();
  gauge_recvbuf->SetValue(0);
}

void ViewerWindow::showOneToOne(bool show_1to1)
{
    this->show_1to1 = show_1to1;
    adjustCanvasSize();
}


void ViewerWindow::grabKeyboard(bool grabit)
{
  if(canvas)
    // grab later on enter/leave
    canvas->do_keyboard_grab = grabit;
}
