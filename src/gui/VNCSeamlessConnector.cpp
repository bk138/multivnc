
#include <wx/log.h>
#include "VNCSeamlessConnector.h"





/*
  public members
*/



VNCSeamlessConnector::VNCSeamlessConnector(wxWindow* parent, VNCConn* c)
{
  conn = c;

  // init all x stuff start
  x_offset=0; y_offset=0;
  pointer_warp_threshold=5;
  grabCursor=0;
  remote_xpos=0.0;
  remote_ypos=0.0;
  pointer_speed = 0.0;
  edge = EDGE_NORTH;
  edge_width=5;
  restingx=-1;
  restingy=-1;
  emulate_wheel=0;
  emulate_nav=0;
  wheel_button_up=4;
  scroll_lines=1;
  mac_mode=0;
  hidden=0;
  last_event_time = 0;
  client_selection_text=0;
  client_selection_text_length=0;
  saved_xpos=-1;
  saved_ypos=-1;
  saved_remote_xpos=-1;
  saved_remote_ypos=-1;
  grab_timeout=0;
  grab_timeout_delay=590;
  requested_desktop = -2;
  current_desktop = -2;
  current_number_of_desktops = -2;
  remote_is_locked = 0;

  debug = true;
  noblank=0;
  resurface = false;
  si.framebufferWidth = c->getFrameBufferWidth();
  si.framebufferHeight = c->getFrameBufferHeight();
  acceleration=1.0;
  no_wakeup_delay = 0x7fffffff;
  // init all x stuff end


  if(CreateXWindow())
    fprintf(stderr, "sucessfully created xwindow!\n");

  adjustSize(); 

  
  // this is like our main loop
  if( Create() != wxTHREAD_NO_ERROR )
    err.Printf(_("Could not create VNC thread!"));

  if( GetThread()->Run() != wxTHREAD_NO_ERROR )
    err.Printf(_("Could not start VNC thread!")); 
}


VNCSeamlessConnector::~VNCSeamlessConnector()
{
  if(GetThread() && GetThread()->IsRunning())
    {
      fprintf(stderr,"VNCSeamlessConnector %p: before thread delete\n", this);
      GetThread()->Delete(); // this blocks if thread is joinable, i.e. on stack
      fprintf(stderr,"VNCSeamlessConnector %p: after thread delete\n", this);
    }
  
  //close window
  XDestroyWindow(dpy,  topLevel);
}



void VNCSeamlessConnector::adjustSize()
{
  wxLogDebug(wxT("VNCSeamlessConnector %p: adjusting size to (%i, %i)"),
	     this,
	     conn->getFrameBufferWidth(),
	     conn->getFrameBufferHeight());

}




/*
  protected members
*/

wxThread::ExitCode VNCSeamlessConnector::Entry()
{
  
  while (! GetThread()->TestDestroy())
    {
      HandleXEvents();
    }
  
  fprintf(stderr, "seamless thread exiting\n");
  return 0;
}





/*
  private members
*/

int warn_about_hotkey(Display *dpy, XErrorEvent *ev)
{
  //grabkeysym=0;
  fprintf(stderr,"Warning: Failed to bind x2vnc hotkey, hotkey disabled.\n");
  return 0;
}




/*
 * AllXEventsPredicate is needed to make XCheckIfEvent return all events.
 */

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
  int i;
  int ew;
  
  Pixmap    nullPixmap;
  XColor    dummyColor;
  
  if (!(dpy = XOpenDisplay(NULL))) {
    fprintf(stderr," unable to open display %s\n",
	    XDisplayName(NULL));
    return False;
  }

  /*
   * check extensions
   */


#ifdef HAVE_MIT_SCREENSAVER
  {
    int x,y;
    server_has_mitscreensaver=XScreenSaverQueryExtension(dpy, &x,&y);
    if(debug)
      fprintf(stderr,"MIT-SCREEN-SAVER = %d\n",server_has_mitscreensaver);
  }
#endif

  if(noblank
#ifdef HAVE_MIT_SCREENSAVER
     && (!server_has_mitscreensaver)
#endif
     )
    fprintf(stderr,"-noblank option used, but no approperiate extensions found:\n"
	    " x2vnc will keep the remote screen active at all times.\n");
  
  for (i = 0; i < 256; i++)
    modifierPressed[i] = False;
  
  /* Try to work out the geometry of the top-level window */
  
  displayWidth = WidthOfScreen(DefaultScreenOfDisplay(dpy));
  displayHeight = HeightOfScreen(DefaultScreenOfDisplay(dpy));
  
  saved_remote_xpos = displayWidth / 2;
  saved_remote_ypos = displayHeight / 2;
  
  if(restingy == -1)
    {
      restingy = si.framebufferHeight -2 - mac_mode;
      restingx = si.framebufferWidth -2 + mac_mode;
    }

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

	    x_offset = heads[besthead].x_org;
	    y_offset = heads[besthead].y_org;
	    displayWidth = heads[besthead].width;
	    displayHeight = heads[besthead].height;
#if 0
	    fprintf(stderr,"[%d,%d-%d,%d]\n",x_offset,y_offset,
		    displayWidth,displayHeight);
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
  wmHints.x=x_offset;
  wmHints.y=y_offset;
  
  switch(edge)
    {
    case EDGE_EAST: wmHints.x=displayWidth-ew+x_offset;
    case EDGE_WEST: topLevelHeight=displayHeight;
      break;
      
    case EDGE_SOUTH: wmHints.y=displayHeight-ew+y_offset;
    case EDGE_NORTH: topLevelWidth=displayWidth;
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

  current_desktop_atom = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
  number_of_desktops_atom = XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", False);

#if 1
  {
    Atom t = XInternAtom(dpy, "_NET_WM_WINDOW_DOCK", False);

    XChangeProperty(dpy,
		    topLevel,
		    XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False),
		    XA_ATOM,
		    32,
		    PropModeReplace,
		    (unsigned char *)&t,
		    1);
  }
#endif


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
  
 

  /* hide the cursor, so user won't get confused
     which keyboard has control */
  wxMouseEvent e;
  e.m_x = restingx;
  e.m_y = restingy;
  conn->sendPointerEvent(e);

  pointer_speed = acceleration * pow( (si.framebufferWidth * si.framebufferHeight) / (float)(displayWidth * displayHeight), 0.25 );



  fprintf(stderr," pointer multiplier: %f\n",pointer_speed);


  /* Compute two identifiable locations, as far as possible from each other */
  if(displayWidth * 2 > displayHeight)
    {
      origo1=wxPoint(displayWidth/3,displayHeight/2);
      origo2=wxPoint(displayWidth*2/3,displayHeight/2);
      origo_separation=displayWidth/3;
    }
  else if(displayHeight * 2 > displayWidth)
    {
      origo1=wxPoint(displayWidth/2,displayHeight/3);
      origo2=wxPoint(displayWidth/2,displayHeight*2/3);
      origo_separation=displayHeight/3;
    }
  else
    {
      int N=(int)( (2*(displayWidth+displayHeight)-sqrt((-3*displayWidth*displayWidth-3*displayHeight*displayHeight+8*displayWidth*displayHeight)) )/7.0 );
      origo1=wxPoint(N,N);
      origo2=wxPoint(displayWidth-N,displayHeight-N);
      origo_separation=N;
    }
  origo1.x+=x_offset;
  origo1.y+=y_offset;
  origo2.x+=x_offset;
  origo2.y+=y_offset;

  /*
    dumpcoord(&origo1);
    dumpcoord(&origo2);
    fprintf(stderr,"offset=%d, %d\n",x_offset,y_offset);
  */
    
  return True;
}




/*
 * check_idle()
 * Check how long the user has been idle using
 * various methods.... (in ms)
 */

Time VNCSeamlessConnector::check_idle(void)
{

#if HAVE_MIT_SCREENSAVER
  if(server_has_mitscreensaver)
    {
      static XScreenSaverInfo* info = 0; 
  
      if (!info) info = XScreenSaverAllocInfo();
    
      XScreenSaverQueryInfo(dpy, DefaultRootWindow(dpy), info);
      return info->idle;
    }
#endif

  return 0;
}





void VNCSeamlessConnector::WiggleMouse(void)
{
  int tmpy=restingy > si.framebufferHeight /2 ? restingy -1 : restingy + 1;

  wxMouseEvent e;
  e.m_x = restingx;
  e.m_y = tmpy;
  conn->sendPointerEvent(e);
  e.m_x = restingx;
  e.m_y = restingy;
  conn->sendPointerEvent(e);
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

      /* fprintf(stderr,"WARP: %d %d\n",next_origo->x, next_origo->y); */
      XWarpPointer(dpy,None,
		   DefaultRootWindow(dpy),0,0,0,0,
		   next_origo->x,
		   next_origo->y);
      motion_events=0;
    }
}





/*
 * HandleXEvents.
 */

Bool VNCSeamlessConnector::HandleXEvents(void)
{
  XEvent ev;

  if(grabbed)
    {
      if(grab_timeout_delay && time(0) > grab_timeout)
	ungrabit(-1, -1, DefaultRootWindow(dpy));
    }else{ /* grabbed */
    int remote_idle = time(0) - last_event_time;
    if(remote_idle > no_wakeup_delay)
      remote_is_locked=1;
#ifdef DEBUG
    fprintf(stderr,"IDLE=%d remote_idle=%d noblank=%d remote_is_locked=%d\n", check_idle(),remote_idle,noblank,remote_is_locked);
#endif
    if(noblank &&
       remote_idle >= 45 &&
       !remote_is_locked &&
       check_idle() < 30000)
      {
	WiggleMouse();
      }
  }
  
  
  /* presumably XCheckMaskEvent is more efficient than XCheckIfEvent -GRM */
  while(XCheckIfEvent(dpy, &ev, AllXEventsPredicate, NULL))
    {
      if (ev.xany.window == topLevel)
	{
	  if (!HandleTopLevelEvent(&ev))
	    return False;
	}
      else if (ev.xany.window == DefaultRootWindow(dpy) ||
	       ev.xany.window == RootWindow(dpy, 0)) {
	if (!HandleRootEvent(&ev))
	  return False;
      }
      else if (ev.type == MappingNotify)
	{
	  XRefreshKeyboardMapping(&ev.xmapping);
	}
    }

  doWarp();

  return True;
}





#define EW (edge == EDGE_EAST || edge==EDGE_WEST)
#define NS (edge == EDGE_NORTH || edge==EDGE_SOUTH)
#define ES (edge == EDGE_EAST || edge==EDGE_SOUTH)
#define NS (edge == EDGE_NORTH || edge==EDGE_SOUTH)

int VNCSeamlessConnector::enter_translate(int isedge, int width, int pos)
{
  if(!isedge) return pos;
  if(ES) return 0;
  return width-1;
}

int VNCSeamlessConnector::leave_translate(int isedge, int width, int pos)
{
  if(!isedge) return pos;
  if(ES) return width-edge_width;
  return 0;
}


#define EDGE(X) ((X)?edge_width:0)

#define SCALEX(X)							\
  ( ((X)-EDGE(edge==EDGE_WEST ))*(si.framebufferWidth-1 )/(displayWidth -1-EDGE(EW)) )

#define SCALEY(Y)							\
  ( ((Y)-EDGE(edge==EDGE_NORTH))*(si.framebufferHeight-1)/(displayHeight-1-EDGE(NS)) )


int VNCSeamlessConnector::sendpointerevent(int x, int y, int buttonmask)
{
  if(mac_mode && (buttonmask & (1<<2)))
    {
      buttonmask &=~ (1<<2);
      buttonmask |=~ (1<<0);
    }

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




void VNCSeamlessConnector::grabit(int x, int y, int state)
{
  Window selection_owner;

  if(hidden)
    {
      XMapRaised(dpy, topLevel);
      hidden=0;
    }

  XGrabPointer(dpy, topLevel, True,
	       PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
	       GrabModeAsync, GrabModeAsync,
	       None, grabCursor, CurrentTime);
  XGrabKeyboard(dpy, topLevel, True, 
		GrabModeAsync, GrabModeAsync,
		CurrentTime);


  grabbed=1;
  next_origo=NULL;
  current_origo=NULL;

  if(x > -1 && y > -1)
    {
      doWarp();

      /* Whut? How can this be right? */
      remote_xpos=SCALEX(x);
      remote_ypos=SCALEY(y);
      sendpointerevent(remote_xpos, remote_ypos, (state & 0x1f00) >> 8);
    }


  mouseOnScreen = 1;

  selection_owner=XGetSelectionOwner(dpy, XA_PRIMARY);
  /*   fprintf(stderr,"Selection owner: %lx\n",(long)selection_owner); */

  if(selection_owner != None && selection_owner != topLevel)
    {
      XConvertSelection(dpy,
			XA_PRIMARY, XA_STRING, XA_CUT_BUFFER0,
			topLevel, CurrentTime);
    }
  SET_GRAB_TIMEOUT();
}

void VNCSeamlessConnector::ungrabit(int x, int y, Window warpWindow)
{
  int i;


  wxMouseEvent e;
  e.m_x = restingx;
  e.m_y = restingy;
  
  conn->sendPointerEvent(e);

  if(x > -1 && y > -1 )
    {
      XWarpPointer(dpy,None, warpWindow, 0,0,0,0, x_offset + x, y_offset + y);
      XFlush(dpy);
    }
  XUngrabKeyboard(dpy, CurrentTime);
  XUngrabPointer(dpy, CurrentTime);
  mouseOnScreen = warpWindow == DefaultRootWindow(dpy);
  XFlush(dpy);
  

  
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



  if(!edge_width) hidewindow();
  
  grabbed=0;
}




void VNCSeamlessConnector::shortsleep(int usec)
{
  struct timeval timeout;
  timeout.tv_sec=0;
  timeout.tv_usec=usec;
  select(0,0,0,0,&timeout);
}


void VNCSeamlessConnector::dumpMotionEvent(XEvent *ev)
{
  fprintf(stderr,"{ %d, %d } -> { %d, %d } = %d, %d\n",
	  current_location.x,
	  current_location.y,
	  XROOT(ev->xmotion),
	  YROOT(ev->xmotion),
	  XROOT(ev->xmotion)-current_location.x,
	  YROOT(ev->xmotion)-current_location.y);
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
  n=displayHeight - a.y;
  if(n < ret) ret=n;
  n=displayWidth - a.x;
  if(n < ret) ret=n;
  return ret;
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

  SET_GRAB_TIMEOUT();

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
	    shortsleep(5000);

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
	  grabit(enter_translate(EW,displayWidth ,XROOT(ev->xcrossing)),
		 enter_translate(NS,displayHeight,YROOT(ev->xcrossing)),
		 ev->xcrossing.state);
	}
      return 1;
      
    case MotionNotify:
      
      if(grabbed)
	{
	  int i, d=0;
	  Window warpWindow;
	  wxPoint offset(0,0);

	  do
	    {

		{
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
		}
	    }
	  while (XCheckTypedWindowEvent(dpy, topLevel, MotionNotify, ev));

	  if(pointer_speed > 1 &&
	     offset.x*offset.x + offset.y*offset.y < 
	     pointer_warp_threshold)
	    {
	      remote_xpos+=offset.x;
	      remote_ypos+=offset.y;
	    }else{
	    remote_xpos+=offset.x * pointer_speed;
	    remote_ypos+=offset.y * pointer_speed;
	  }

#if 0
	  fprintf(stderr," ==> {%f, %f} (%d,%d)\n",
		  remote_xpos, remote_ypos,
		  si.framebufferWidth,
		  si.framebufferHeight);
#endif
	
	  if(!(ev->xmotion.state & 0x1f00))
	    {
	      warpWindow = DefaultRootWindow(dpy);
	      switch(edge)
		{
		case EDGE_NORTH: 
		  d=remote_ypos >= si.framebufferHeight;
		  y = edge_width;  /* FIXME */
		  x = remote_xpos * displayWidth / si.framebufferWidth;
		  break;
		case EDGE_SOUTH:
		  d=remote_ypos < 0;
		  y = displayHeight - edge_width -1; /* FIXME */
		  x = remote_xpos * displayWidth / si.framebufferWidth;
		  break;
		case EDGE_EAST:
		  d=remote_xpos < 0;
		  x = displayWidth -  edge_width -1;  /* FIXME */
		  y = remote_ypos * displayHeight /si.framebufferHeight ;
		  break;
		case EDGE_WEST:
		  d=remote_xpos > si.framebufferWidth;
		  x = edge_width;  /* FIXME */
		  y = remote_ypos * displayHeight / si.framebufferHeight;
		  break;
		}
	    }

	  if(d)
	    {
	      if(x<0) x=0;
	      if(y<0) y=0;
	      if(y>=displayHeight) y=displayHeight-1;
	      if(x>=displayWidth) x=displayWidth-1;
	      ungrabit(x, y, warpWindow);
	      return 1;
	    }else{
	    if(remote_xpos < 0) remote_xpos=0;
	    if(remote_ypos < 0) remote_ypos=0;

	    if(remote_xpos >= si.framebufferWidth)
	      remote_xpos=si.framebufferWidth-1;

	    if(remote_ypos >= si.framebufferHeight)
	      remote_ypos=si.framebufferHeight-1;

	    i=sendpointerevent((int)remote_xpos,
			       (int)remote_ypos,
			       (ev->xmotion.state & 0x1f00) >> 8);
	  }

	}
      return 1;
	
    case ButtonPress:
    case ButtonRelease:
      if (emulate_wheel &&
	  ev->xbutton.button >= 4 &&
	  ev->xbutton.button <= 5)
	{
	  int l;
	  if (ev->xbutton.button == wheel_button_up)
	    ks = XK_Up;
	  else
	    ks = XK_Down;

	  if(ev->type == ButtonPress)
	    {
	      for(l=1;l<scroll_lines;l++)
		{
		  //SendKeyEvent(ks, 1); /* keydown */
		  //SendKeyEvent(ks, 0); /* keyup */
		  wxKeyEvent key_event;
		  key_event.m_keyCode = ks;
		  conn->sendKeyEvent(key_event, true, false);
		  conn->sendKeyEvent(key_event, false, false);
		}
	    }


	  //SendKeyEvent(ks, ev->type == ButtonPress); /* keydown */
	  wxKeyEvent key_event;
	  key_event.m_keyCode = ks;
	  conn->sendKeyEvent(key_event, ev->type == ButtonPress, false); /* keydown */
	  break;
	}

      if (emulate_nav &&
	  ev->xbutton.button >= 6 &&
	  ev->xbutton.button <= 7)
	{
	  int ctrlcode;
	  if (ev->xbutton.button == 6)
	    ks = XK_Left;
	  else
	    ks = XK_Right;


	  ctrlcode=XKeysymToKeycode(dpy, XK_Alt_L);

	  //SendKeyEvent(XK_Alt_L, ev->type == ButtonPress);
	  wxKeyEvent key_event;
	  key_event.m_keyCode = XK_Alt_L;
	  conn->sendKeyEvent(key_event, ev->type == ButtonPress, false); /* keydown */

	  modifierPressed[ctrlcode]=ev->type == ButtonPress;

	  //SendKeyEvent(ks, ev->type == ButtonPress); /* keydown */
	  key_event.m_keyCode = ks;
	  conn->sendKeyEvent(key_event, ev->type == ButtonPress, false); /* keydown */

	  break;
	}

      if(mac_mode && ev->xbutton.button == 3)
	{
	  int ctrlcode=XKeysymToKeycode(dpy, XK_Control_L);

	  //SendKeyEvent(XK_Control_L, ev->type == ButtonPress);
	  wxKeyEvent key_event;
	  key_event.m_keyCode = XK_Control_L;
	  conn->sendKeyEvent(key_event, ev->type == ButtonPress, false); 


	  modifierPressed[ctrlcode]=ev->type == ButtonPress;
	}

      if (ev->type == ButtonPress) {
	buttonMask = (((ev->xbutton.state & 0x1f00) >> 8) |
		      (1 << (ev->xbutton.button - 1)));
      } else {
	buttonMask = (((ev->xbutton.state & 0x1f00) >> 8) &
		      ~(1 << (ev->xbutton.button - 1)));
      }
      
      return sendpointerevent(remote_xpos,
			      remote_ypos,
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

	if(debug)
	  fprintf(stderr,"  --> %x (%c) name=%s (%s)\n",ks,ks,keyname,
		  ev->type == KeyPress ? "down" : "up");

	/* We assume that typing means an unlocked display */
	remote_is_locked=0;

	//      return SendKeyEvent(ks, (ev->type == KeyPress));
	wxKeyEvent key_event;
	key_event.m_keyCode = ks;
	conn->sendKeyEvent(key_event, ev->type == KeyPress, true); 
	return 1;
      }
      
    case ClientMessage:
      if ((ev->xclient.message_type == wmProtocols) &&
	  (ev->xclient.data.l[0] == wmDeleteWindow))
	{
	  exit(0);
	}
      break;
    }

  return True;
}






int VNCSeamlessConnector::get_root_int_prop(Atom property)
{
  unsigned char *data=0;
  Atom t;
  int format;
  unsigned long len;
  unsigned long bytes_left;
  int ret=XGetWindowProperty(dpy,
			     DefaultRootWindow(dpy),
			     property,
			     0,
			     1,
			     0, /* delete */
			     XA_CARDINAL,
			     &t,
			     &format,
			     &len,
			     &bytes_left,
			     &data);

  ret = *(uint32_t *)data;
  
  if(data)
    XFree(data);

  return ret;
}

void VNCSeamlessConnector::check_desktop(void)
{
  int t=requested_desktop;

  current_desktop=get_root_int_prop(current_desktop_atom);
  
  switch(t)
    {
    case -2: t=current_desktop; break;
    case -1:
      t=get_root_int_prop(number_of_desktops_atom)-1;
      if(t<0) t=0;
      break;
    }
  if( t == current_desktop)
    {
      mapwindow();
    }else{
    hidewindow();
  }
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
	exit(0);
#endif
      break;
    
    case KeyPress:
      {
	KeySym ks;
	char keyname[256];
	keyname[0] = '\0';
	XLookupString(&ev->xkey, keyname, 256, &ks, NULL);
#ifdef DEBUG
	fprintf(stderr,"ROOT: Pressing %x (%c) name=%s  code=%d\n",ks,ks,keyname,ev->xkey.keycode);
#endif
	if(ev->xkey.keycode)
	  {
	    saved_xpos=XROOT(ev->xkey);
	    saved_ypos=YROOT(ev->xkey);
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
	    x = XROOT(ev->xcrossing);
	    y = XROOT(ev->xcrossing);
	    if (!nowOnScreen) {
	      x = enter_translate(EW,displayWidth,XROOT(ev->xcrossing));
	      y = enter_translate(NS,displayHeight,YROOT(ev->xcrossing));
	    }
	    switch(edge)
	      {
	      case EDGE_NORTH:
		grab=y < displayHeight / 2;
		break;
	      case EDGE_SOUTH:
		grab=y > displayHeight / 2;
		break;
	      case EDGE_EAST:
		grab=x > displayWidth / 2;
		break;
	      case EDGE_WEST:
		grab=x < displayWidth / 2;
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
	  grabit(enter_translate(EW,displayWidth ,XROOT(ev->xcrossing)),
		 enter_translate(NS,displayHeight,YROOT(ev->xcrossing)),
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
      if(ev->xproperty.atom == current_desktop_atom ||
	 ev->xproperty.atom == number_of_desktops_atom)
	check_desktop();

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



