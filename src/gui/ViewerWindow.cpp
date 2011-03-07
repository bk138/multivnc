

#include <wx/sizer.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/dcclient.h>
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
  wxTimer update_timer;
  void onUpdateTimer(wxTimerEvent& event);
  void onPaint(wxPaintEvent &event);
  void onMouseAction(wxMouseEvent &event);
  void onKeyDown(wxKeyEvent &event);
  void onKeyUp(wxKeyEvent &event);
  void onChar(wxKeyEvent &event);
  void onFocusLoss(wxFocusEvent &event);

protected:
  DECLARE_EVENT_TABLE();

public:
  VNCCanvas(wxWindow* parent, VNCConn* c);

  VNCConn* conn;
  wxRegion updated_area;
};
	


#define VNCCANVAS_UPDATE_TIMER_ID 1
#define VNCCANVAS_UPDATE_TIMER_INTERVAL 30

BEGIN_EVENT_TABLE(VNCCanvas, wxPanel)
    EVT_PAINT  (VNCCanvas::onPaint)
    EVT_MOUSE_EVENTS (VNCCanvas::onMouseAction)
    EVT_KEY_DOWN (VNCCanvas::onKeyDown)
    EVT_KEY_UP (VNCCanvas::onKeyUp)
    EVT_CHAR (VNCCanvas::onChar)
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
 
  // this kinda cursor creation works everywhere
  wxBitmap vnccursor_bitmap(vnccursor_bits, 16, 16);
  wxBitmap vnccursor_mask_bitmap(vnccursor_mask, 16, 16);
  vnccursor_bitmap.SetMask(new wxMask(vnccursor_mask_bitmap));
  wxImage vnccursor_image = vnccursor_bitmap.ConvertToImage();
  vnccursor_image.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, 8);
  vnccursor_image.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, 8);
  SetCursor(wxCursor(vnccursor_image));

  update_timer.SetOwner(this, VNCCANVAS_UPDATE_TIMER_ID);
  update_timer.Start(VNCCANVAS_UPDATE_TIMER_INTERVAL);

  // SetSize() isn't enough...
  SetInitialSize(wxSize(c->getFrameBufferWidth(), c->getFrameBufferHeight()));
  CentreOnParent();
  parent->Layout();
}



/*
  private members
*/

void VNCCanvas::onUpdateTimer(wxTimerEvent& event)
{
#ifdef __WXDEBUG__
  wxLongLong t0 = wxGetLocalTimeMillis();
  size_t nr_rects = 0;
  size_t nr_bytes = 0;
#endif
  
  wxClientDC dc(this);
  
  // get the update rect list
  wxRegionIterator upd(updated_area); 
  while(upd)
    {
      wxRect update_rect(upd.GetRect());
     
      wxLogDebug(wxT("VNCCanvas %p: drawing updated rect: (%i,%i,%i,%i)"),
		 this,
		 update_rect.x,
		 update_rect.y,
		 update_rect.width,
		 update_rect.height);
#ifdef __WXDEBUG__      
      ++nr_rects;
      nr_bytes += update_rect.width * update_rect.height;
#endif

      const wxBitmap& region = conn->getFrameBufferRegion(update_rect);
      if(region.IsOk())
	dc.DrawBitmap(region, update_rect.x, update_rect.y);
	
      ++upd;
    }

  updated_area.Clear();


#ifdef __WXDEBUG__
  wxLongLong t1 = wxGetLocalTimeMillis();
  wxLogDebug(wxT("VNCCanvas %p: updating %d rects (%d bytes) took %lld ms"),
	     this,
	     nr_rects,
	     nr_bytes,
	     (t1-t0).GetValue());
#endif
}


void VNCCanvas::onPaint(wxPaintEvent &WXUNUSED(event))
{
  // this happens on GTK even if our size is (0,0)
  if(GetSize().GetWidth() == 0 || GetSize().GetHeight() == 0)
    return;

  wxPaintDC dc(this);

  // get the update rect list
  wxRegionIterator upd(GetUpdateRegion()); 
  while(upd)
    {
      wxRect update_rect(upd.GetRect());
     
      wxLogDebug(wxT("VNCCanvas %p: got repaint event: (%i,%i,%i,%i)"),
		 this,
		 update_rect.x,
		 update_rect.y,
		 update_rect.width,
		 update_rect.height);
      
    
      const wxBitmap& region = conn->getFrameBufferRegion(update_rect);
      if(region.IsOk())
	dc.DrawBitmap(region, update_rect.x, update_rect.y);
	
      ++upd;
    }
}






void VNCCanvas::onMouseAction(wxMouseEvent &event)
{
  if(event.Entering())
    {
      SetFocus();
      
      wxCriticalSectionLocker lock(wxGetApp().mutex_theclipboard); 
      
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
END_EVENT_TABLE()


/*
  constructor/destructor
 */

ViewerWindow::ViewerWindow(wxWindow* parent, VNCConn* conn):
  wxPanel(parent)
{
  /*
    setup man window
  */
  // a sizer dividing the window vertically
  SetSizer(new wxBoxSizer(wxVERTICAL));

  // the upper subwindow
  canvas_container = new wxScrolledWindow(this);
  canvas_container->SetScrollRate(VIEWERWINDOW_SCROLL_RATE, VIEWERWINDOW_SCROLL_RATE);
  GetSizer()->Add(canvas_container, 1, wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);


  // the lower subwindow
  stats_container = new wxScrolledWindow(this);
  stats_container->SetScrollRate(VIEWERWINDOW_SCROLL_RATE, VIEWERWINDOW_SCROLL_RATE);
  GetSizer()->Add(stats_container, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALL, 3);
  


  /*
    setup upper window
  */
  canvas_container->SetSizer(new wxBoxSizer(wxHORIZONTAL));
  canvas = new VNCCanvas(canvas_container, conn);
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
  label_updrawbytes = new wxStaticText(stats_container, wxID_ANY, _("Raw KB/s:"));
  text_ctrl_updrawbytes = new wxTextCtrl(stats_container, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80,18), wxTE_READONLY);
  label_updcount = new wxStaticText(stats_container, wxID_ANY, _("Updates/s:"));
  text_ctrl_updcount = new wxTextCtrl(stats_container, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80,18), wxTE_READONLY);
  label_latency = new wxStaticText(stats_container, wxID_ANY, _("Latency ms:"));
  text_ctrl_latency = new wxTextCtrl(stats_container, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80,18), wxTE_READONLY);
  label_lossratio = new wxStaticText(stats_container, wxID_ANY, _("Loss Ratio:"));
  text_ctrl_lossratio = new wxTextCtrl(stats_container, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80,18), wxTE_READONLY);
  label_recvbuf = new wxStaticText(stats_container, wxID_ANY, _("Rcv Buffer:"));
  gauge_recvbuf = new wxGauge(stats_container, wxID_ANY, 10, wxDefaultPosition, wxSize(80,18), wxGA_HORIZONTAL|wxGA_SMOOTH);

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
      const VNCConn* c = canvas->conn;

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
      
      if( ! c->getUpdRawByteStats().IsEmpty() )
	*text_ctrl_updrawbytes << wxAtoi(c->getUpdRawByteStats().Last().AfterLast(wxT(',')))/1024;
      if( ! c->getUpdCountStats().IsEmpty() )
	*text_ctrl_updcount << c->getUpdCountStats().Last().AfterLast(wxT(','));
      if( ! c->getLatencyStats().IsEmpty() )
	*text_ctrl_latency << c->getLatencyStats().Last().AfterLast(wxT(','));
      if( ! c->getMCLossRatioStats().IsEmpty() )
	*text_ctrl_lossratio << c->getMCLossRatioStats().Last().AfterLast(wxT(','));

      gauge_recvbuf->SetRange(c->getMCBufSize());
      gauge_recvbuf->SetValue(c->getMCBufFill());

      // flash red when buffer full
      if(gauge_recvbuf->GetRange() == gauge_recvbuf->GetValue())
	label_recvbuf->SetForegroundColour(*wxRED);
      else
	label_recvbuf->SetForegroundColour(dflt_fg);
    }
}



/*
  public members
*/


void ViewerWindow::adjustCanvasSize()
{
  if(canvas)
    {
      wxLogDebug(wxT("ViewerWindow %p: adjusting canvas size to (%i, %i)"),
		 this,
		 canvas->conn->getFrameBufferWidth(),
		 canvas->conn->getFrameBufferHeight());

      // SetSize() isn't enough...
      canvas->SetInitialSize(wxSize(canvas->conn->getFrameBufferWidth(), canvas->conn->getFrameBufferHeight()));
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
  Layout();

  text_ctrl_updrawbytes->Clear();
  text_ctrl_updcount->Clear();
  text_ctrl_latency->Clear();
  text_ctrl_lossratio->Clear();
  gauge_recvbuf->SetValue(0);
}
