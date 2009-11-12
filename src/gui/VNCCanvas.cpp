

#include <wx/sizer.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include "VNCCanvas.h"
#include "MultiVNCApp.h"
#include "res/vnccursor.xbm"
#include "res/vnccursor-mask.xbm"




/********************************************

  VNCCanvas class

********************************************/


BEGIN_EVENT_TABLE(VNCCanvas, wxPanel)
    EVT_PAINT  (VNCCanvas::onPaint)
    EVT_MOUSE_EVENTS (VNCCanvas::onMouseAction)
    EVT_KEY_DOWN (VNCCanvas::onKeyDown)
    EVT_KEY_UP (VNCCanvas::onKeyUp)
    EVT_CHAR (VNCCanvas::onChar)
END_EVENT_TABLE();



/*
  constructor/destructor
*/

VNCCanvas::VNCCanvas(wxWindow* parent, VNCConn* c):
  wxPanel(parent, -1, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS)
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
      
    
      wxBitmap region = conn->getFrameBufferRegion(update_rect);
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




/*
  public members
*/

void VNCCanvas::drawRegion(wxRect& rect)
{
  wxLogDebug(wxT("VNCCanvas %p: drawing region (%i,%i,%i,%i)"),
	     this,
	     rect.x,
	     rect.y,
	     rect.width,
	     rect.height);

  wxClientDC dc(this);
  wxBitmap region = conn->getFrameBufferRegion(rect);
  dc.DrawBitmap(region, rect.x, rect.y);
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
