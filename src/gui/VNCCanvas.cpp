

#include <wx/sizer.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include "res/vnccursor.xbm"
#include "res/vnccursor-mask.xbm"
#include "MultiVNCApp.h"
#include "VNCCanvas.h"



/********************************************

  VNCCanvas class

********************************************/


BEGIN_EVENT_TABLE(VNCCanvas, wxPanel)
    EVT_PAINT  (VNCCanvas::onPaint)
    EVT_MOUSE_EVENTS (VNCCanvas::onMouseAction)
    EVT_KEY_DOWN (VNCCanvas::onKeyDown)
    EVT_KEY_UP (VNCCanvas::onKeyUp)
    EVT_CHAR (VNCCanvas::onChar)
    EVT_KILL_FOCUS(VNCCanvas::onFocusLoss)
    EVT_VNCCONNUPDATENOTIFY (wxID_ANY, VNCCanvas::onVNCConnUpdateNotify)
END_EVENT_TABLE();



/*
  constructor/destructor 
  (make sure size is set to 0,0 ow win32 gets stuck sending 
  paint events in listen mode)
*/

VNCCanvas::VNCCanvas(wxWindow* parent, VNCConn* c):
  wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(0,0), wxWANTS_CHARS)
{
  conn = c;
  adjustSize(); 
 
  // this kinda cursor creation works everywhere
  wxBitmap vnccursor_bitmap(vnccursor_bits, 16, 16);
  wxBitmap vnccursor_mask_bitmap(vnccursor_mask, 16, 16);
  vnccursor_bitmap.SetMask(new wxMask(vnccursor_mask_bitmap));
  wxImage vnccursor_image = vnccursor_bitmap.ConvertToImage();
  vnccursor_image.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, 8);
  vnccursor_image.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, 8);
  SetCursor(wxCursor(vnccursor_image));
}



/*
  private members
*/

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



void VNCCanvas::onVNCConnUpdateNotify(VNCConnUpdateNotifyEvent& event)
{
  VNCConn* sending_conn = static_cast<VNCConn*>(event.GetEventObject());

  // only do something if this is our VNCConn
  if(sending_conn == conn)
    drawRegion(event.rect);
}


/*
  public members
*/

void VNCCanvas::drawRegion(wxRect& rect)
{
#ifdef __WXDEBUG__
  wxLongLong t0 = wxGetLocalTimeMillis();
#endif

  wxClientDC dc(this);

  const wxBitmap& region = conn->getFrameBufferRegion(rect);
  dc.DrawBitmap(region, rect.x, rect.y);

#ifdef __WXDEBUG__
  wxLongLong t1 = wxGetLocalTimeMillis();
  wxLogDebug(wxT("VNCCanvas %p: drawing region (%i,%i,%i,%i) size %d took %lld ms"),
	     this,
	     rect.x,
	     rect.y,
	     rect.width,
	     rect.height,
	     rect.width * rect.height,
	     (t1-t0).GetValue());
#endif

}



void VNCCanvas::adjustSize()
{
  wxLogDebug(wxT("VNCCanvas %p: adjusting size to (%i, %i)"),
	     this,
	     conn->getFrameBufferWidth(),
	     conn->getFrameBufferHeight());

  // SetSize() isn't enough...
  SetInitialSize(wxSize(conn->getFrameBufferWidth(), conn->getFrameBufferHeight()));

  CentreOnParent();
  GetParent()->Layout();
}






/********************************************

  VNCCanvasContainer class

********************************************/

#define VNCCANVASCONTAINER_SCROLL_RATE 10


/*
  constructor/destructor
 */

VNCCanvasContainer::VNCCanvasContainer(wxWindow* parent):
  wxScrolledWindow(parent)
{
  canvas = 0;
  SetScrollRate(VNCCANVASCONTAINER_SCROLL_RATE, VNCCANVASCONTAINER_SCROLL_RATE);
  SetSizer(new wxBoxSizer(wxHORIZONTAL));
}




VNCCanvasContainer::~VNCCanvasContainer()
{
  if(canvas)
    delete canvas;
}




void VNCCanvasContainer::setCanvas(VNCCanvas* c)
{
  canvas = c;

  wxBoxSizer* sizer_vert = new wxBoxSizer(wxVERTICAL);
  sizer_vert->Add(c, 3, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL);
  GetSizer()->Add(sizer_vert, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL);
}



VNCCanvas* VNCCanvasContainer::getCanvas() const
{
  return canvas;
}
