
#include <wx/clipbrd.h>
#include <wx/log.h>
#include <wx/display.h>
#ifdef __WXGTK__
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#endif
#include "MultiVNCApp.h"
#include "VNCSeamlessConnector.h"




/********************************************

  VNCSeamlessConnector class

********************************************/


/*
  public members
*/

#define RAISETIMER_ID 666
BEGIN_EVENT_TABLE(VNCSeamlessConnector, wxFrame)
  EVT_TIMER   (RAISETIMER_ID, VNCSeamlessConnector::onRaiseTimer)
END_EVENT_TABLE();



#define EDGE_EW (edge == EDGE_EAST || edge==EDGE_WEST)
#define EDGE_NS (edge == EDGE_NORTH || edge==EDGE_SOUTH)
#define EDGE_ES (edge == EDGE_EAST || edge==EDGE_SOUTH)
#define EDGE_NS (edge == EDGE_NORTH || edge==EDGE_SOUTH)


VNCSeamlessConnector::VNCSeamlessConnector(wxWindow* parent, VNCConn* c, int e, size_t ew, float accel)
  : wxFrame(parent, wxID_ANY, c->getDesktopName(), wxDefaultPosition, wxDefaultSize,
	    wxFRAME_SHAPED | wxBORDER_NONE | wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP)
{
  conn = c;
  edge = e;
  edge_width = ew;
  acceleration = accel;

  pointer_warp_threshold=5;

  pointer_speed = 0.0;
  grabbed = false;

  /*
  // init all x2vnc stuff start
  grabCursor=0;
  hidden=0;
  client_selection_text=0;
  client_selection_text_length=0;
  saved_xpos=-1;
  saved_ypos=-1;
  debug = true;
  resurface = false;
  // init all x2vnc stuff end
 for (int i = 0; i < 256; i++)
    modifierPressed[i] = False;
 
  if (!(dpy = XOpenDisplay(NULL))) 
    fprintf(stderr," unable to open display %s\n",  XDisplayName(NULL));

  
  // this sets: topLevel, deskopt atoms
  //if(CreateXWindow())
  // fprintf(stderr, "sucessfully created xwindow!\n");
  // topLevel = GDK_WINDOW_XID(GetHandle()->window);


  topLevel = 0;
  //runtimer.SetOwner(this, 666);
  //runtimer.Start(5);
  */

  adjustSize(); 

#ifdef __WXGTK__
  // this is needed cause since newer gtk version the gnome panel 
  // raises above the connector sometimes...grrr
  raisetimer.SetOwner(this, RAISETIMER_ID);
  raisetimer.Start(100);
#endif

  canvas = new VNCSeamlessConnectorCanvas(this);
#ifdef NDEBUG
  canvas->SetCursor(wxCursor(wxCURSOR_BLANK));
#endif
  wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(canvas, 1, wxEXPAND, 0);
  SetSizer(sizer);
  Layout();


  this->Show(true);
  // seems that has to come after Show() to take effect
  SetBackgroundColour(*wxRED);
  SetTransparent(100);
}


VNCSeamlessConnector::~VNCSeamlessConnector()
{
  /*
  runtimer.Stop();
  
  if(topLevel)
    XDestroyWindow(dpy,  topLevel);
  */
}


bool VNCSeamlessConnector::isSupportedByCurrentPlatform() {
#ifdef __WXGTK__
    wxString sessionType;
    wxGetEnv("XDG_SESSION_TYPE", &sessionType);
    return sessionType.IsSameAs("x11");
#endif
    return false;
}



void VNCSeamlessConnector::adjustSize()
{
  wxLogDebug(wxT("VNCSeamLessConnector %p: adjusting size"), this);

  // use the biggest screen
  wxSize biggest_size;
  for(size_t i=0; i < wxDisplay::GetCount(); ++i)
    {
      wxSize this_size = wxDisplay(i).GetGeometry().GetSize();

      if(this_size.GetWidth() * this_size.GetHeight() >
	 biggest_size.GetWidth() * biggest_size.GetHeight())
	{
	  biggest_size = this_size;
	  display_index = i;
	}
    }
  // set offset accordingly
  multiscreen_offset = wxDisplay(display_index).GetGeometry().GetPosition();

  // get local display size
  display_size = wxDisplay(display_index).GetGeometry().GetSize();

  /*
  saved_remote_xpos = display_size.GetWidth() / 2;
  saved_remote_ypos = display_size.GetHeight() / 2;
  */
  
  // get remote display size
  framebuffer_size.SetWidth(conn->getFrameBufferWidth());
  framebuffer_size.SetHeight(conn->getFrameBufferHeight());

  // compute a pos to hide cursor, so user won't get confused
  // which keyboard has control 
  remoteParkingPos.x = framebuffer_size.GetWidth() -2;
  remoteParkingPos.y = framebuffer_size.GetHeight() -2;

  wxMouseEvent e;
  e.m_x = remoteParkingPos.x;
  e.m_y = remoteParkingPos.y;
  conn->sendPointerEvent(e);

  // calc pointer speed from sizes
  pointer_speed = acceleration * pow( (framebuffer_size.GetWidth() * framebuffer_size.GetHeight()) / (float)(display_size.GetWidth() * display_size.GetHeight()), 0.25 );

  wxLogDebug(wxT("VNCSeamlessConnector %p: pointer multiplier %f"), this, pointer_speed);



  // Compute two identifiable locations, as far as possible from each other 
  if(display_size.GetWidth() * 2 > display_size.GetHeight())
    {
      origo1=wxPoint(display_size.GetWidth()/3,display_size.GetHeight()/2);
      origo2=wxPoint(display_size.GetWidth()*2/3,display_size.GetHeight()/2);
      origo_separation=display_size.GetWidth()/3;
    }
  else if(display_size.GetHeight() * 2 > display_size.GetWidth())
    {
      origo1=wxPoint(display_size.GetWidth()/2,display_size.GetHeight()/3);
      origo2=wxPoint(display_size.GetWidth()/2,display_size.GetHeight()*2/3);
      origo_separation=display_size.GetHeight()/3;
    }
  else
    {
      int N=(int)( (2*(display_size.GetWidth()+display_size.GetHeight())-
		    sqrt((-3*display_size.GetWidth()*display_size.GetWidth()-3*display_size.GetHeight()*display_size.GetHeight()+8*display_size.GetWidth()*display_size.GetHeight()))
		    )/7.0 );
      origo1=wxPoint(N,N);
      origo2=wxPoint(display_size.GetWidth()-N,display_size.GetHeight()-N);
      origo_separation=N;
    }
  origo1.x+=multiscreen_offset.x;
  origo1.y+=multiscreen_offset.y;
  origo2.x+=multiscreen_offset.x;
  origo2.y+=multiscreen_offset.y;

  wxLogDebug(wxT("VNCSeamlessConnector %p: Origo1 (%d, %d)"), this, origo1.x,origo1.y);
  wxLogDebug(wxT("VNCSeamlessConnector %p: Origo2 (%d, %d)"), this, origo2.x,origo2.y);
  wxLogDebug(wxT("VNCSeamlessConnector %p: multiscreen offset: (%d, %d)"), this, multiscreen_offset.x,multiscreen_offset.y);

  

  // the following code sizes and places our window
  int ew = edge_width;
  if(!ew) 
    ew=1;

  int width=ew;
  int height=ew;
  int x = multiscreen_offset.x;
  int y = multiscreen_offset.y;
  
  switch(edge)
    {
    case EDGE_EAST: x = display_size.GetWidth() - ew + multiscreen_offset.x;
    case EDGE_WEST: height = display_size.GetHeight();
      break;
      
    case EDGE_SOUTH: y = display_size.GetHeight() - ew + multiscreen_offset.y;
    case EDGE_NORTH: width = display_size.GetWidth();
      break;

    default:
      width = height = 0; 
    }
  

  SetSize(width, height);
  Move(x, y);

#ifdef __WXGTK__
  gtk_window_set_type_hint(GTK_WINDOW(GetHandle()), GDK_WINDOW_TYPE_HINT_DOCK);
  gtk_window_set_keep_above(GTK_WINDOW(GetHandle()), true);	
#endif  

   /*
  if(edge_width)
    Raise();
  else
  hidden=1;*/
}



/*
  protected members
*/





/*
  private members
*/

void VNCSeamlessConnector::onRaiseTimer(wxTimerEvent& event)
{
  Raise();
}


void VNCSeamlessConnector::handleMouse(wxMouseEvent& event)
{
  wxPoint evt_root_pos = ClientToScreen(event.GetPosition());
  wxLogDebug(wxT("VNCSeamLessConnector %p: new mouse evt: (%d, %d)"), this, evt_root_pos.x, evt_root_pos.y);

  int x,y;

  doWarp();

  if(event.Entering())
    {
      //  if(!HasCapture())
      wxLogDebug(wxT("VNCSeamlessConnector %p: mouse entering!"), this);

      // set cuttext from clipboard
      {
	if(wxTheClipboard->Open()) 
	  {
	    if(wxTheClipboard->IsSupported(wxDF_TEXT))
	      {
		wxTextDataObject data;
		wxTheClipboard->GetData(data);
		wxString text = data.GetText();
		wxLogDebug(wxT("VNCSeamlessConnector %p: setting cuttext: '%s'"), this, text.c_str());
		conn->setCuttext(text);
	      }
	    wxTheClipboard->Close();
	  }
      }

      // check for a change of display size
      if(display_size != wxDisplay(display_index).GetGeometry().GetSize())
	adjustSize();

      if(!grabbed)
	{
	  wxLogDebug(wxT("VNCSeamlessConnector %p: mouse entering, grabbing!"), this);
	  grabit(enter_translate(EDGE_EW,display_size.GetWidth() , (evt_root_pos.x - multiscreen_offset.x)),
		 enter_translate(EDGE_NS,display_size.GetHeight(), (evt_root_pos.y - multiscreen_offset.y)),
		 0);
	  //	     ev->xcrossing.state); //FIXME buttons
	}
      return;
    }

  if(event.Moving() || event.Dragging())
    {
      wxLogDebug(wxT("VNCSeamlessConnector %p: mouse moving/dragging!"), this);
      if(grabbed)
	{
	  wxLogDebug(wxT("VNCSeamlessConnector %p: mouse moving/dragging while grabbed!"), this);
	  int d=0;
	  
	  wxPoint offset(0,0);
		
	  wxPoint new_location(evt_root_pos.x, evt_root_pos.y);
	    
	  motion_events++;
	    
	  if(next_origo &&
	     (coord_dist_sq(new_location, *next_origo) < coord_dist_sq(new_location, current_location) ||
	      coord_dist_from_edge(new_location) < motion_events))
	    {
	      current_origo=next_origo;
	      current_location=*next_origo;
	      next_origo=NULL;
	      motion_events=0;
	    }
	    
	  current_speed = new_location - current_location;
	  if(current_origo)
	    offset= offset + current_speed;
	  current_location=new_location;
		
	  
	  if(pointer_speed > 1 &&
	     offset.x*offset.x + offset.y*offset.y < 
	     pointer_warp_threshold)
	    {
	      remotePos.x+=offset.x;
	      remotePos.y+=offset.y;
	      wxLogDebug(wxT("VNCSeamlessConnector %p: if: remotePos (%f, %f)"), this, remotePos.x, remotePos.y);
	    }
	  else
	    {
	      remotePos.x+=offset.x * pointer_speed;
	      remotePos.y+=offset.y * pointer_speed;
	      wxLogDebug(wxT("VNCSeamlessConnector %p: else: remotePos (%f, %f)"), this, remotePos.x, remotePos.y);
	    }

	
	  // if(!(ev->xmotion.state & 0x1f00)) //FIXME
	  if(!event.Dragging())
	    {
	      switch(edge)
		{
		case EDGE_NORTH: 
		  d=remotePos.y >= framebuffer_size.GetHeight();
		  y = edge_width;  /* FIXME */
		  x = remotePos.x * display_size.GetWidth() / framebuffer_size.GetWidth();
		  break;
		case EDGE_SOUTH:
		  d=remotePos.y < 0;
		  y = display_size.GetHeight() - edge_width -1; /* FIXME */
		  x = remotePos.x * display_size.GetWidth() / framebuffer_size.GetWidth();
		  break;
		case EDGE_EAST:
		  d=remotePos.x < 0;
		  x = display_size.GetWidth() -  edge_width -1;  /* FIXME */
		  y = remotePos.y * display_size.GetHeight() /framebuffer_size.GetHeight() ;
		  break;
		case EDGE_WEST:
		  d=remotePos.x > framebuffer_size.GetWidth();
		  x = edge_width;  /* FIXME */
		  y = remotePos.y * display_size.GetHeight() / framebuffer_size.GetHeight();
		  break;
		}
	    }

	  if(d)
	    {
	      wxLogDebug(wxT("VNCSeamlessConnector %p: d is set!"), this);
	      if(x<0) x=0;
	      if(y<0) y=0;
	      if(y>=display_size.GetHeight()) y=display_size.GetHeight()-1;
	      if(x>=display_size.GetWidth()) x=display_size.GetWidth()-1;
	      ungrabit(x, y);
	      return;
	    }
	  else
	    {
	      if(remotePos.x < 0) remotePos.x=0;
	      if(remotePos.y < 0) remotePos.y=0;
	      
	      if(remotePos.x >= framebuffer_size.GetWidth())
		remotePos.x=framebuffer_size.GetWidth()-1;
	      
	      if(remotePos.y >= framebuffer_size.GetHeight())
		remotePos.y=framebuffer_size.GetHeight()-1;
	      
	      event.m_x = remotePos.x;
	      event.m_y = remotePos.y;
	      wxLogDebug(wxT("VNCSeamlessConnector %p: moving/dragging sending (%d, %d)"), this, event.m_x, event.m_y);
	      conn->sendPointerEvent(event);
	    }

	}
      return;
    }

  // a non-moving event (button, wheel, whatever...)
  event.m_x = remotePos.x;
  event.m_y = remotePos.y;
  conn->sendPointerEvent(event);
  return;
}




void VNCSeamlessConnector::handleKey(wxKeyEvent& evt)
{
  doWarp(); 
		      
  wxLogDebug(wxT("VNCSeamlessConnector %p: got some key event!"), this);

  if(evt.GetEventType() == wxEVT_KEY_DOWN)
    conn->sendKeyEvent(evt, true, false);

  if(evt.GetEventType() == wxEVT_KEY_UP)
    conn->sendKeyEvent(evt, false, false);

  if(evt.GetEventType() == wxEVT_CHAR)
    conn->sendKeyEvent(evt, true, true);
}



void VNCSeamlessConnector::handleFocusLoss()
{
  wxLogDebug(wxT("VNCSeamlessConnector %p: lost focus, upping key modifiers"), this);
 
  wxKeyEvent key_event;

  key_event.m_keyCode = WXK_SHIFT;
  conn->sendKeyEvent(key_event, false, false);
  key_event.m_keyCode = WXK_ALT;
  conn->sendKeyEvent(key_event, false, false);
  key_event.m_keyCode = WXK_CONTROL;
  conn->sendKeyEvent(key_event, false, false);
}


void VNCSeamlessConnector::handleCaptureLoss()
{
  wxLogDebug(wxT("VNCSeamlessConnector %p: lost capture, emergency ungrab"), this);

  ungrabit(display_size.x/2, display_size.y/2);
  handleFocusLoss();
}


void VNCSeamlessConnector::doWarp(void)
{
  if(grabbed)
    {
      if(next_origo) return;
      if(current_origo &&
	 current_location.x == current_origo->x &&
	 current_location.y == current_origo->y)
	return;

      if(current_origo == &origo1)
	next_origo=&origo2;
      else
	next_origo=&origo1;
      
      wxLogDebug(wxT("VNCSeamlessConnector %p: WARP to (%d, %d)"), this, next_origo->x, next_origo->y);
      /*XWarpPointer(dpy,None,
		   DefaultRootWindow(dpy),0,0,0,0,
		   next_origo->x,
		   next_origo->y);*/
      

      wxPoint warp_pos= canvas->ScreenToClient(*next_origo);
      wxLogDebug(wxT("VNCSeamlessConnector %p: WARP translated (%d, %d)"), this, warp_pos.x, warp_pos.y);
      canvas->WarpPointer(warp_pos.x, warp_pos.y);

      motion_events=0;
    }
}



int VNCSeamlessConnector::enter_translate(int isedge, int width, int pos)
{
  if(!isedge)
    return pos;
  if(EDGE_ES)
    return 0;
  return width-1;
}

int VNCSeamlessConnector::leave_translate(int isedge, int width, int pos)
{
  if(!isedge) 
    return pos;
  if(EDGE_ES)
    return width-edge_width;
  return 0;
}


void VNCSeamlessConnector::grabit(int x, int y, int state)
{
  //Window selection_owner;

  wxLogDebug(wxT("VNCSeamlessConnector %p: GRAB!"), this);

  /*
  if(hidden)
    {
      XMapRaised(dpy, topLevel);
      hidden=0;
    }

  if(!topLevel)
  */
    {
      canvas->CaptureMouse();
      canvas->SetFocus();
#ifdef __WXGTK__     
      gdk_keyboard_grab(gtk_widget_get_window(canvas->GetHandle()), False, GDK_CURRENT_TIME);
#endif
    }
    /*
  else
    {
      XGrabPointer(dpy, topLevel, True,
	       PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
	       GrabModeAsync, GrabModeAsync,
	       None, grabCursor, CurrentTime);
      XGrabKeyboard(dpy, topLevel, True, 
		    GrabModeAsync, GrabModeAsync,
		    CurrentTime);
    }
    */

  grabbed=1;
  next_origo=NULL;
  current_origo=NULL;

  if(x > -1 && y > -1)
    {
      doWarp();

#define EDGE(X) ((X)?edge_width:0)
#define SCALEX(X)							\
  ( ((X)-EDGE(edge==EDGE_WEST ))*(framebuffer_size.GetWidth()-1 )/(display_size.GetWidth() -1-EDGE(EDGE_EW)) )
#define SCALEY(Y)							\
  ( ((Y)-EDGE(edge==EDGE_NORTH))*(framebuffer_size.GetHeight()-1)/(display_size.GetHeight()-1-EDGE(EDGE_NS)) )

      /* Whut? How can this be right? */
      remotePos.x=SCALEX(x);
      remotePos.y=SCALEY(y);
      wxMouseEvent evt;
      evt.m_x = remotePos.x;
      evt.m_y = remotePos.y;
      wxLogDebug(wxT("VNCSeamlessConnector %p: GRAB got position (%d, %d)"), this, x, y);
      wxLogDebug(wxT("VNCSeamlessConnector %p: GRAB sending pointer event (%d, %d)"), this, evt.m_x, evt.m_y);
      //sendpointerevent(remotePos.x, remotePos.y, (state & 0x1f00) >> 8);
      conn->sendPointerEvent(evt);
    }


  //  mouseOnScreen = 1;


  /*
  selection_owner=XGetSelectionOwner(dpy, XA_PRIMARY);
  //   fprintf(stderr,"Selection owner: %lx\n",(long)selection_owner); 

  if(selection_owner != None && selection_owner != topLevel)
    {
      XConvertSelection(dpy,
			XA_PRIMARY, XA_STRING, XA_CUT_BUFFER0,
			topLevel, CurrentTime);
			}
  */
  //  XSync(dpy, False);
}

void VNCSeamlessConnector::ungrabit(int x, int y)
{
  // int i;

  wxMouseEvent e;
  e.m_x = remoteParkingPos.x;
  e.m_y = remoteParkingPos.y;
  
  conn->sendPointerEvent(e);

  wxLogDebug(wxT("VNCSeamlessConnector %p: UNGRAB!"), this);
    

  if(x > -1 && y > -1 )
    {
      //XWarpPointer(dpy,None, warpWindow, 0,0,0,0, multiscreen_offset.x + x, multiscreen_offset.y + y);
      //XFlush(dpy);
      
      wxPoint warp_pos= ScreenToClient(wxPoint(multiscreen_offset.x+x, multiscreen_offset.y+y));
      canvas->WarpPointer(warp_pos.x, warp_pos.y);

      wxLogDebug(wxT("VNCSeamlessConnector %p: UNGRAB  WARP!"), this);
    }

  /* if(topLevel)
    {
      XUngrabKeyboard(dpy, CurrentTime);
      XUngrabPointer(dpy, CurrentTime);
    }
    else*/
    {
#ifdef __WXGTK__    
      gdk_keyboard_ungrab(GDK_CURRENT_TIME);
#endif
      canvas->ReleaseMouse();
    }

    /*
  mouseOnScreen = warpWindow == DefaultRootWindow(dpy);
  XFlush(dpy);
    */

    /*  
  for (i = 255; i >= 0; i--)
    {
      if (modifierPressed[i]) 
	{
	  wxKeyEvent key_event;

	  key_event.m_keyCode = WXK_SHIFT;
	  conn->sendKeyEvent(key_event, false, false);
	  key_event.m_keyCode = WXK_ALT;
	  conn->sendKeyEvent(key_event, false, false);
	  key_event.m_keyCode = WXK_CONTROL;
	  conn->sendKeyEvent(key_event, false, false);
	
	  //if (!SendKeyEvent(XKeycodeToKeysym(dpy, i, 0), False))
	  //  return;
	  modifierPressed[i]=False;
	}
    }
    */


    //if(!edge_width) hidewindow();
  
  grabbed=0;
}


int VNCSeamlessConnector::coord_dist_sq(wxPoint a, wxPoint b)
{
  a= a-b;
  return a.x*a.x + a.y*a.y;
}

int VNCSeamlessConnector::coord_dist_from_edge(wxPoint a)
{
  int n,ret=a.x;
  if(a.y < ret) ret=a.y;
  n=display_size.GetHeight() - a.y;
  if(n < ret) ret=n;
  n=display_size.GetWidth() - a.x;
  if(n < ret) ret=n;
  return ret;
}





/********************************************

  VNCSeamlessConnectorCanvas class

********************************************/

BEGIN_EVENT_TABLE(VNCSeamlessConnectorCanvas, wxPanel)
  EVT_MOUSE_EVENTS(VNCSeamlessConnectorCanvas::onMouse)
  EVT_KEY_DOWN (VNCSeamlessConnectorCanvas::onKeyDown)
  EVT_KEY_UP (VNCSeamlessConnectorCanvas::onKeyUp)
  EVT_CHAR (VNCSeamlessConnectorCanvas::onChar)
  EVT_KILL_FOCUS(VNCSeamlessConnectorCanvas::onFocusLoss)
  EVT_MOUSE_CAPTURE_LOST(VNCSeamlessConnectorCanvas::onCaptureLoss)
END_EVENT_TABLE();


VNCSeamlessConnectorCanvas::VNCSeamlessConnectorCanvas(VNCSeamlessConnector* parent)
  : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS)
{
    p = parent;
    SetBackgroundColour(*wxGREEN);
}


void VNCSeamlessConnectorCanvas::onMouse(wxMouseEvent &event)
{
  p->handleMouse(event);
}



void VNCSeamlessConnectorCanvas::onKeyDown(wxKeyEvent &event)
{
  p->handleKey(event);
}


void VNCSeamlessConnectorCanvas::onKeyUp(wxKeyEvent &event)
{
  p->handleKey(event);
}


void VNCSeamlessConnectorCanvas::onChar(wxKeyEvent &event)
{
  p->handleKey(event);
}

void VNCSeamlessConnectorCanvas::onFocusLoss(wxFocusEvent &event)
{
  p->handleFocusLoss();
}

void VNCSeamlessConnectorCanvas::onCaptureLoss(wxMouseCaptureLostEvent &event)
{
  p->handleCaptureLoss();
}











#if 0


/*
  x2vnc stuff
*/


void VNCSeamlessConnector::onRuntimer(wxTimerEvent& event)
{
  //doWarp();


  HandleXEvents();
}



Bool
AllXEventsPredicate(Display *dpy, XEvent *ev, char *arg)
{
  return True;
}

Bool VNCSeamlessConnector::CreateXWindow(void)
{
  XSetWindowAttributes attr;
  char defaultGeometry[256];
  XSizeHints wmHints;
  int ew;
  int topLevelWidth, topLevelHeight;
  Pixmap    nullPixmap;
  XColor    dummyColor;
 

  /*
   * check extensions
   */
#ifdef HAVE_XINERAMA
  {
    int x,y;
    if(XineramaQueryExtension(dpy,&x,&y) &&
       XineramaIsActive(dpy))
      {
	int pos,e;
	int num_heads;
	int bestpos=0;
	int besthead=0;
	XineramaScreenInfo *heads;

	if(heads=XineramaQueryScreens(dpy, &num_heads))
	  {
	    /* Loop over all heads and find whatever head
	     * corresponds best with the edge the user has
	     * selected. If the user selects "north", the
	     * bigger screen will normally be selected.
	     *
	     * This is actually kind of stupid, it should really
	     * use the biggest screen for off-screen stuff.
	     */
       
	    for(e=0;e<num_heads;e++)
	      {
		switch(edge)
		  {
		  case EDGE_EAST:
		    pos=heads[e].x_org + heads[e].width + (heads[e].height>>8);
		    break;
		  case EDGE_WEST:
		    pos=1000-heads[e].x_org + (heads[e].height>>8);
		    break;
		  case EDGE_SOUTH:
		    pos=heads[e].y_org + heads[e].height + (heads[e].width>>8);
		    break;
		  case EDGE_NORTH:
		    pos=1000-heads[e].y_org + (heads[e].width>>8);
		    break;
		  }
		fprintf(stderr,"screen[%d] pos=%d\n",e,pos);

		if(pos > bestpos)
		  {
		    bestpos=pos;
		    besthead=e;
		  }
	      }


	    printf("Xinerama detected, x2vnc will use screen %d.\n",
		   besthead+1);

	    multiscreen_offset.x = heads[besthead].x_org;
	    multiscreen_offset.y = heads[besthead].y_org;
	    display_size.SetWidth(heads[besthead].width);
	    display_size.SetHeight(heads[besthead].height);
#if 0
	    fprintf(stderr,"[%d,%d-%d,%d]\n",multiscreen_offset.x,multiscreen_offset.y,
		    display_size.GetWidth(),display_size.GetHeight());
#endif
	  }
	XFree(heads);
      }
  }
#endif

  ew=edge_width;
  if(!ew) ew=1;
  topLevelWidth=ew;
  topLevelHeight=ew;
  wmHints.x=multiscreen_offset.x;
  wmHints.y=multiscreen_offset.y;
  
  switch(edge)
    {
    case EDGE_EAST: wmHints.x=display_size.GetWidth()-ew+multiscreen_offset.x;
    case EDGE_WEST: topLevelHeight=display_size.GetHeight();
      break;
      
    case EDGE_SOUTH: wmHints.y=display_size.GetHeight()-ew+multiscreen_offset.y;
    case EDGE_NORTH: topLevelWidth=display_size.GetWidth();
      break;
    }
  
  wmHints.flags = PMaxSize | PMinSize |PPosition |PBaseSize;
  
  wmHints.min_width = topLevelWidth;
  wmHints.min_height = topLevelHeight;
  
  wmHints.max_width = topLevelWidth;
  wmHints.max_height = topLevelHeight;
  
  wmHints.base_width = topLevelWidth;
  wmHints.base_height = topLevelHeight;
  
  sprintf(defaultGeometry, "%dx%d+%d+%d",
	  topLevelWidth, topLevelHeight,
	  wmHints.x, wmHints.y);
  
  XWMGeometry(dpy, DefaultScreen(dpy), NULL, defaultGeometry, 0,
	      &wmHints, &wmHints.x, &wmHints.y,
	      &topLevelWidth, &topLevelHeight, &wmHints.win_gravity);
  
  /* Create the top-level window */
  
  attr.border_pixel = 0; /* needed to allow 8-bit cmap on 24-bit display -
			    otherwise we get a Match error! */
  attr.background_pixel = BlackPixelOfScreen(DefaultScreenOfDisplay(dpy));
  attr.event_mask = ( LeaveWindowMask|
		      StructureNotifyMask|
		      ButtonPressMask|
		      ButtonReleaseMask|
		      PointerMotionMask|
		      KeyPressMask|
		      KeyReleaseMask|
		      EnterWindowMask|
		      (resurface?VisibilityChangeMask:0) );
  
  attr.override_redirect=1;

  topLevel = XCreateWindow(dpy, DefaultRootWindow(dpy), wmHints.x, wmHints.y,
			   topLevelWidth, topLevelHeight, 0, CopyFromParent,
			   InputOutput, CopyFromParent,
			   (CWBorderPixel|
			    CWEventMask|
			    CWOverrideRedirect|
			    CWBackPixel),
			   &attr);


  Atom t = XInternAtom(dpy, "_NET_WM_WINDOW_DOCK", False);
  
  XChangeProperty(dpy,
		  topLevel,
		  XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False),
		  XA_ATOM,
		  32,
		  PropModeReplace,
		    (unsigned char *)&t,
		  1);

  wmHints.flags |= USPosition; /* try to force WM to place window */
  XSetWMNormalHints(dpy, topLevel, &wmHints);

  wmProtocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
  wmDeleteWindow = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(dpy, topLevel, &wmDeleteWindow, 1);
  
  

  if(edge_width)
    XMapRaised(dpy, topLevel);
  else
    hidden=1;
  
  /*
   * For multi-screen setups, we need to know if the mouse is on the 
   * screen.
   * - GRM
   */
  if (ScreenCount(dpy) > 1) {
    Window root, child;
    int root_x, root_y;
    int win_x, win_y;
    unsigned int keys_buttons;
    
    XSelectInput(dpy, DefaultRootWindow(dpy),
		 PropertyChangeMask |
		 EnterWindowMask |
		 LeaveWindowMask |
		 KeyPressMask);
    /* Cut buffer happens on screen 0 only. */
    if (DefaultRootWindow(dpy) != RootWindow(dpy, 0)) {
      XSelectInput(dpy, RootWindow(dpy, 0), PropertyChangeMask);
    }
    XQueryPointer(dpy, DefaultRootWindow(dpy), &root, &child,
		  &root_x, &root_y, &win_x, &win_y, &keys_buttons);
    mouseOnScreen = root == DefaultRootWindow(dpy);
  } else {
    XSelectInput(dpy, DefaultRootWindow(dpy), PropertyChangeMask);
    mouseOnScreen = 1;
  }

#ifdef HAVE_XRANDR
  {
    int major=0, minor=0;

    if(XRRQueryVersion(dpy, &major, &minor) && major >= 1)
      {
	XRRQueryExtension(dpy, &xrandr_event_base, &xrandr_error_base);
	XRRSelectInput(dpy, DefaultRootWindow(dpy), RRScreenChangeNotifyMask);
	server_has_randr=1;
      }
  }
#endif
  
  nullPixmap = XCreatePixmap(dpy, DefaultRootWindow(dpy), 1, 1, 1);
  if(!debug)
    grabCursor = 
      XCreatePixmapCursor(dpy, nullPixmap, nullPixmap,
			  &dummyColor, &dummyColor, 0, 0);
  
 

    
  return True;
}






/*
 * HandleXEvents.
 */

Bool VNCSeamlessConnector::HandleXEvents(void)
{
  

    
  XEvent ev;

 
  /* presumably XCheckMaskEvent is more efficient than XCheckIfEvent -GRM */
  while(XCheckIfEvent(dpy, &ev, AllXEventsPredicate, NULL))
    {
      if (ev.xany.window == topLevel)
	{
	  if (!HandleTopLevelEvent(&ev))
	    return False;
	}
      /*      else if (ev.xany.window == DefaultRootWindow(dpy) ||
	       ev.xany.window == RootWindow(dpy, 0)) {
	if (!HandleRootEvent(&ev))
	  return False;
	  }*/
      /*      else if (ev.type == MappingNotify)
	      {
	      XRefreshKeyboardMapping(&ev.xmapping);
	      }*/
    }


  doWarp();

  return True;
}





int VNCSeamlessConnector::sendpointerevent(int x, int y, int buttonmask)
{
  wxMouseEvent e;
  e.m_x = x;
  e.m_y = y;

  if(buttonmask & rfbButton1Mask)
    e.m_leftDown = true;

  if(buttonmask & rfbButton2Mask)
    e.m_middleDown = true;
  
  if(buttonmask & rfbButton3Mask)
    e.m_rightDown = true;

  if(buttonmask & rfbWheelUpMask)
    e.m_wheelRotation = 1;
  
  if(buttonmask & rfbWheelDownMask)
    e.m_wheelRotation = -1;

  conn->sendPointerEvent(e);
  return 1;
}


void VNCSeamlessConnector::mapwindow(void)
{
  if(edge_width)
    {
      hidden=0;
      XMapRaised(dpy, topLevel);
    }
}

void VNCSeamlessConnector::hidewindow(void)
{
  hidden=1;
  XUnmapWindow(dpy, topLevel);
}







void VNCSeamlessConnector::dumpMotionEvent(XEvent *ev)
{
  fprintf(stderr,"{ %d, %d } -> { %d, %d } = %d, %d\n",
	  current_location.x,
	  current_location.y,
	  ev->xmotion.x_root - multiscreen_offset.x,
	  ev->xmotion.y_root- multiscreen_offset.y,
	  (ev->xmotion.x_root - multiscreen_offset.x) - current_location.x,
	  (ev->xmotion.y_root - multiscreen_offset.y)-current_location.y);
}




/*
 * HandleTopLevelEvent.
 */
Bool VNCSeamlessConnector::HandleTopLevelEvent(XEvent *ev)
{
  int x, y;
  
  int buttonMask;
  KeySym ks;
  static Atom COMPOUND_TEXT;
  /* Atom: requestor asks for a list of supported targets (GRM 24 Oct 2003) */
  static Atom TARGETS;

  switch (ev->type)
    {
    case SelectionRequest:
      {
	XEvent reply;

	XSelectionRequestEvent *req=& ev->xselectionrequest;

	reply.xselection.type=SelectionNotify;
	reply.xselection.display=req->display;
	reply.xselection.selection=req->selection;
	reply.xselection.requestor=req->requestor;
	reply.xselection.target=req->target;
	reply.xselection.time=req->time;
	reply.xselection.property=None;
      
	if(COMPOUND_TEXT == None)
	  COMPOUND_TEXT = XInternAtom(dpy, "COMPOUND_TEXT", True);

	/* Initialize TARGETS atom if required (GRM 24 Oct 2003) */
	if(TARGETS == None) TARGETS = XInternAtom(dpy, "TARGETS", True);

	if(client_selection_text)
	  {
	    if(req->target == TARGETS)
	      {
		/*
		 * Requestor is asking what targets we support. Reply with what we
		 * support (XA_STRING and COMPOUND_TEXT).
		 *
		 * (GRM 24 Oct 2003)
		 */
		Atom targets[2];
		if(req->property == None)
		  req->property = TARGETS;

		targets[0] = XA_STRING;
		targets[1] = COMPOUND_TEXT;
		XChangeProperty(dpy,
				req->requestor,
				req->property,
				XA_ATOM,
				32,
				PropModeReplace,
				(unsigned char *) targets,
				2);

		reply.xselection.property=req->property;
	      }
	    else if(req->target == XA_STRING || req->target == COMPOUND_TEXT)
	      {
		if(req->property == None)
		  req->property = XA_CUT_BUFFER0;

#if 0
		fprintf(stderr,"Setting property %d on window %x to: %s (len=%d)\n",req->property, req->requestor, client_selection_text,client_selection_text_length);
#endif

		XChangeProperty(dpy,
				req->requestor,
				req->property,
				req->target,
				8,
				PropModeReplace,
				(const unsigned char*)client_selection_text,
				client_selection_text_length);

		reply.xselection.property=req->property;
	  
	      }
	    else if(debug)
	      {
		/*
		 * Requestor is asking for something we don't understand. Complain.
		 * (GRM 24 Oct 2003)
		 */
		char *name;
		name = XGetAtomName(dpy, req->target);
		fprintf(stderr, "Unknown property target request %s\n", name);
		XFree(name);
	      }
	  }
	XSendEvent(dpy, req->requestor, 0,0, &reply);
	XFlush (dpy);
      }
      return 1;

    case SelectionNotify:
      {
	Atom t;
	unsigned long len=0;
	unsigned long bytes_left=0;
	int format;
	unsigned char *data=0;
	int ret;

	ret=XGetWindowProperty(dpy,
			       topLevel,
			       XA_CUT_BUFFER0,
			       0,
			       0x100000,
			       1, /* delete */
			       XA_STRING,
			       &t,
			       &format,
			       &len,
			       &bytes_left,
			       &data);

	if(format == 8)
	  conn->setCuttext( wxString((char *)data, wxConvUTF8));
	//SendClientCutText((char *)data, len);
#ifdef DEBUG
	fprintf(stderr,"GOT selection info: ret=%d type=%d fmt=%d len=%d bytes_left=%d %p '%s'\n",
		ret, (int)t,format,len,bytes_left,data,data);
#endif
	if(data) XFree(data);
			 
      }
      return 1;
      
    case VisibilityNotify:
      /*
       * I avoid resurfacing when the window becomes fully obscured, because
       * that *probably* means that somebody is running xlock.
       * Please tell me if you have a problem with this.
       * - Hubbe
       */
      if (ev->xvisibility.state == VisibilityPartiallyObscured &&
	  resurface && !hidden)
	{
	  static long last_resurface=0;
	  long t=time(0);

	  if(t == last_resurface)
	    wxMilliSleep(5);

	  last_resurface=t;
#ifdef DEBUG
	  fprintf(stderr,"Raising window!\n");
#endif
	  mapwindow();
	  /* XRaiseWindow(dpy, topLevel); */
	}
      return 1;

      /*
       * We don't need to worry about multi-screen here; that's handled
       * below, as a root event.
       * - GRM
       */
    case EnterNotify:
      if(!grabbed && ev->xcrossing.mode==NotifyNormal)
	{
	  grabit(enter_translate(EDGE_EW,display_size.GetWidth() , (ev->xcrossing.x_root - multiscreen_offset.x)),
		 enter_translate(EDGE_NS,display_size.GetHeight(), (ev->xcrossing.y_root - multiscreen_offset.y)),
		 ev->xcrossing.state);
	}
      return 1;
      
    case MotionNotify:
      
      if(grabbed)
	{
	  int i, d=0;
	  Window warpWindow;
	  wxPoint offset(0,0);

	  
		
	  wxPoint new_location(ev->xmotion.x_root, ev->xmotion.y_root);
	  if(debug) 
	    {
	      dumpMotionEvent(ev);
	      
	      if(next_origo)
		fprintf(stderr," {%d} <? {%d}\n",coord_dist_sq(new_location, *next_origo), coord_dist_sq(new_location, current_location));
	    }
	    
	  motion_events++;
	    
	  if(next_origo &&
	     (coord_dist_sq(new_location, *next_origo) < coord_dist_sq(new_location, current_location) ||
	      coord_dist_from_edge(new_location) < motion_events))
	    {
	      current_origo=next_origo;
	      current_location=*next_origo;
	      next_origo=NULL;
	      motion_events=0;
	    }
	    
	  current_speed = new_location - current_location;
	  if(current_origo)
	    offset= offset + current_speed;
	  current_location=new_location;
		
	  
	  if(pointer_speed > 1 &&
	     offset.x*offset.x + offset.y*offset.y < 
	     pointer_warp_threshold)
	    {
	      remotePos.x+=offset.x;
	      remotePos.y+=offset.y;
	    }else{
	    remotePos.x+=offset.x * pointer_speed;
	    remotePos.y+=offset.y * pointer_speed;
	  }

#if 0
	  fprintf(stderr," ==> {%f, %f} (%d,%d)\n",
		  remotePos.x, remotePos.y,
		  framebuffer_size.GetWidth(),
		  framebuffer_size.GetHeight());
#endif
	
	  if(!(ev->xmotion.state & 0x1f00))
	    {
	      fprintf(stderr, "d gets set!!!\n");
	      warpWindow = DefaultRootWindow(dpy);
	      switch(edge)
		{
		case EDGE_NORTH: 
		  d=remotePos.y >= framebuffer_size.GetHeight();
		  y = edge_width;  /* FIXME */
		  x = remotePos.x * display_size.GetWidth() / framebuffer_size.GetWidth();
		  break;
		case EDGE_SOUTH:
		  d=remotePos.y < 0;
		  y = display_size.GetHeight() - edge_width -1; /* FIXME */
		  x = remotePos.x * display_size.GetWidth() / framebuffer_size.GetWidth();
		  break;
		case EDGE_EAST:
		  d=remotePos.x < 0;
		  x = display_size.GetWidth() -  edge_width -1;  /* FIXME */
		  y = remotePos.y * display_size.GetHeight() /framebuffer_size.GetHeight() ;
		  break;
		case EDGE_WEST:
		  d=remotePos.x > framebuffer_size.GetWidth();
		  x = edge_width;  /* FIXME */
		  y = remotePos.y * display_size.GetHeight() / framebuffer_size.GetHeight();
		  break;
		}
	    }

	  if(d)
	    {
	      fprintf(stderr, "d is set!!!\n");
	      if(x<0) x=0;
	      if(y<0) y=0;
	      if(y>=display_size.GetHeight()) y=display_size.GetHeight()-1;
	      if(x>=display_size.GetWidth()) x=display_size.GetWidth()-1;
	      ungrabit(x, y, warpWindow);
	      return 1;
	    }else{
	    if(remotePos.x < 0) remotePos.x=0;
	    if(remotePos.y < 0) remotePos.y=0;

	    if(remotePos.x >= framebuffer_size.GetWidth())
	      remotePos.x=framebuffer_size.GetWidth()-1;

	    if(remotePos.y >= framebuffer_size.GetHeight())
	      remotePos.y=framebuffer_size.GetHeight()-1;

	    i=sendpointerevent((int)remotePos.x,
			       (int)remotePos.y,
			       (ev->xmotion.state & 0x1f00) >> 8);
	  }

	}
      return 1;
	
    case ButtonPress:
    case ButtonRelease:
      if (ev->type == ButtonPress) {
	buttonMask = (((ev->xbutton.state & 0x1f00) >> 8) |
		      (1 << (ev->xbutton.button - 1)));
      } else {
	buttonMask = (((ev->xbutton.state & 0x1f00) >> 8) &
		      ~(1 << (ev->xbutton.button - 1)));
      }
      
      return sendpointerevent(remotePos.x,
			      remotePos.y,
			      buttonMask);
	
    case KeyPress:
    case KeyRelease:
      {
	char keyname[256];
	keyname[0] = '\0';


	XLookupString(&ev->xkey, keyname, 256, &ks, NULL);
	/*      fprintf(stderr,"Pressing %x (%c) name=%s  code=%d\n",ks,ks,keyname,ev->xkey.keycode); */

	if (IsModifierKey(ks)) {
	  ks = XKeycodeToKeysym(dpy, ev->xkey.keycode, 0);

	  /* Ignore AltGr key, it is handled by XLookupString */
	  if( ks == XK_Mode_switch ) return True;

	  modifierPressed[ev->xkey.keycode] = (ev->type == KeyPress);
	} else {
	  /* This fixes the 'shift-tab' problem - Hubbe */
	  switch(ks)
	    {
#if XK_ISO_Left_Tab != 0x1000ff74
	    case 0x1000ff74: /* HP-UX */
#endif
	    case XK_ISO_Left_Tab: ks=XK_Tab; break;
	    }
	}

	//	if(debug)
	//  fprintf(stderr,"  --> %x (%c) name=%s (%s)\n",ks,ks,keyname,  ev->type == KeyPress ? "down" : "up");


	//      return SendKeyEvent(ks, (ev->type == KeyPress));
	wxKeyEvent key_event;
	key_event.m_keyCode = ks;
	conn->sendKeyEvent(key_event, ev->type == KeyPress, true); 
	return 1;
      }
      
      break;
    }

  return True;
}




/*
 * HandleRootEvent.
 */

Bool VNCSeamlessConnector::HandleRootEvent(XEvent *ev)
{
  char *str;
  int len;
  
  Bool nowOnScreen;
  Bool grab;
  
  int x, y;
  
  switch (ev->type)
    {
    default:
#ifdef HAVE_XRANDR
      if(ev->type == RRScreenChangeNotify + xrandr_event_base)
	;//exit(0);
#endif
      break;
    
    case KeyPress:
      {
	KeySym ks;
	char keyname[256];
	keyname[0] = '\0';
	XLookupString(&ev->xkey, keyname, 256, &ks, NULL);

	//fprintf(stderr,"ROOT: Pressing %x (%c) name=%s  code=%d\n",ks,ks,keyname,ev->xkey.keycode);

	if(ev->xkey.keycode)
	  {
	    saved_xpos=(ev->xkey.x_root - multiscreen_offset.x);
	    saved_ypos=(ev->xkey.y_root - multiscreen_offset.y);
	    grabit(saved_remote_xpos, saved_remote_ypos, 0);
	  }
	break;
      }

    case EnterNotify:
    case LeaveNotify:
      /*
       * Ignore the event if it's due to leaving our window. This will
       * be after an ungrab.
       */
      if (ev->xcrossing.subwindow == topLevel &&
          !ev->xcrossing.same_screen) {
	break;
      }
      
      grab = 0;
      if(!grabbed)
	{
	  nowOnScreen =  ev->xcrossing.same_screen;
	  if (mouseOnScreen != nowOnScreen) {
	    /*
	     * Mouse has left, or entered, the screen. We must grab if
	     * the mouse is now near the edge we're watching.
	     * 
	     * If we do grab, the mouse coordinates are left alone.
	     * The test, however, depends on whether the mouse entered
	     * or left the screen.
	     * 
	     * - GRM
	     */
	    x = (ev->xcrossing.x_root - multiscreen_offset.x);
	    y = (ev->xcrossing.y_root - multiscreen_offset.y);
	    if (!nowOnScreen) {
	      x = enter_translate(EDGE_EW,display_size.GetWidth(),(ev->xcrossing.x_root - multiscreen_offset.x));
	      y = enter_translate(EDGE_NS,display_size.GetHeight(),(ev->xcrossing.y_root - multiscreen_offset.y));
	    }
	    switch(edge)
	      {
	      case EDGE_NORTH:
		grab=y < display_size.GetHeight() / 2;
		break;
	      case EDGE_SOUTH:
		grab=y > display_size.GetHeight() / 2;
		break;
	      case EDGE_EAST:
		grab=x > display_size.GetWidth() / 2;
		break;
	      case EDGE_WEST:
		grab=x < display_size.GetWidth() / 2;
		break;
	      }
	  }
	  mouseOnScreen = nowOnScreen;
	}
      
      /*
       * Do not grab if this is the result of an ungrab
       * or grab (caused by us, usually).
       * 
       * - GRM
       */
      if(grab && ev->xcrossing.mode == NotifyNormal)
	{
	  grabit(enter_translate(EDGE_EW,display_size.GetWidth() ,(ev->xcrossing.x_root - multiscreen_offset.x)),
		 enter_translate(EDGE_NS,display_size.GetHeight(),(ev->xcrossing.y_root - multiscreen_offset.y)),
		 ev->xcrossing.state);
	}
      break;
      
    case PropertyNotify:
      if (ev->xproperty.atom == XA_CUT_BUFFER0)
	{
	  str = XFetchBytes(dpy, &len);
	  if (str) {
#ifdef DEBUG
	    fprintf(stderr,"GOT CUT TEXT: \"%s\"\n",str);
#endif
	    //if (!SendClientCutText(str, len))
	    //  return False;
	    conn->setCuttext(wxString(str, wxConvUTF8));
	    XFree(str);
	  }
	}
     
      break;
    }
  
  return True;
}

void VNCSeamlessConnector::handle_cut_text(char *str, size_t len)
{
  XWindowAttributes   attrs;
  
  if(client_selection_text)
    free((char *)client_selection_text);
  
  client_selection_text_length=len;
  client_selection_text = str;
  
  XGetWindowAttributes(dpy, RootWindow(dpy, 0), &attrs);
  XSelectInput(dpy, RootWindow(dpy, 0),
	       attrs.your_event_mask & ~PropertyChangeMask);
  XStoreBytes(dpy, str, len);
  XSetSelectionOwner(dpy, XA_PRIMARY, topLevel, CurrentTime);
  XSelectInput(dpy, RootWindow(dpy, 0), attrs.your_event_mask);
}

#endif

