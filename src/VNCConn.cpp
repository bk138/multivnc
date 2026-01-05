/* 
   VNCConn.cpp: VNC connection class implementation.

   This file is part of MultiVNC, a multicast-enabled crossplatform 
   VNC viewer.
 
   Copyright (C) 2009, 2010 Christian Beier <dontmind@freeshell.org>
 
   MultiVNC is free software; you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation; either version 2 of the License, or 
   (at your option) any later version. 
 
   MultiVNC is distributed in the hope that it will be useful, 
   but WITHOUT ANY WARRANTY; without even the implied warranty of 
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
   GNU General Public License for more details. 
 
   You should have received a copy of the GNU General Public License 
   along with this program; if not, write to the Free Software 
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. 
*/


#include <cstdarg>
#include <cerrno>
#include <wx/file.h>
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/socket.h>
#ifdef __WIN32__
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif
#include "VNCConn.h"

bool VNCConn::libsshtunnel_initialized = false;

// use some global address
#define VNCCONN_OBJ_ID (void*)VNCConn::thread_got_update

// logfile name
#define LOGFILE _T("MultiVNC.log")


// pixelformat defaults
// seems 8,3,4 and 5,3,2 are possible with rfbGetClient()
#define BITSPERSAMPLE 8
#define SAMPLESPERPIXEL 3
#define BYTESPERPIXEL 4 


// define our new notify events!
DEFINE_EVENT_TYPE(VNCConnListenNOTIFY)
DEFINE_EVENT_TYPE(VNCConnInitNOTIFY)
wxDEFINE_EVENT(VNCConnGetPasswordNOTIFY, wxCommandEvent);
wxDEFINE_EVENT(VNCConnGetCredentialsNOTIFY, wxCommandEvent);
DEFINE_EVENT_TYPE(VNCConnIncomingConnectionNOTIFY)
DEFINE_EVENT_TYPE(VNCConnDisconnectNOTIFY)
DEFINE_EVENT_TYPE(VNCConnUpdateNOTIFY)
DEFINE_EVENT_TYPE(VNCConnFBResizeNOTIFY)
DEFINE_EVENT_TYPE(VNCConnCuttextNOTIFY) 
DEFINE_EVENT_TYPE(VNCConnBellNOTIFY) 
DEFINE_EVENT_TYPE(VNCConnUniMultiChangedNOTIFY);
DEFINE_EVENT_TYPE(VNCConnReplayFinishedNOTIFY);


BEGIN_EVENT_TABLE(VNCConn, wxEvtHandler)
    EVT_TIMER (wxID_ANY, VNCConn::on_stats_timer)
END_EVENT_TABLE();


/*
  constructor/destructor
*/


VNCConn::VNCConn(void* p) : condition_auth(mutex_auth)
{
  // save our caller
  parent = p;
  
  cl = 0;
  multicastStatus = 0;
  latency = -1;

  rfbClientLog = rfbClientErr = thread_logger;

  if (!libsshtunnel_initialized) {
      // this is not thread-safe so run here once
      if (ssh_tunnel_init() == 0) {
          libsshtunnel_initialized = true;
      } else  {
          err.Printf(_("libsshtunnel initialization failed"));
      }
  }
  ssh_tunnel = NULL;

  // statistics stuff
  do_stats = false;
  upd_bytes = 0;
  upd_bytes_inflated = 0;
  upd_count = 0;
  stats_timer.SetOwner(this);

  // fastrequest stuff
  fastrequest_interval = 0;

  // record/replay stuff
  recording = replaying = false;

  require_auth = false;
}




VNCConn::~VNCConn()
{
  Shutdown();
  wxLogDebug(wxT("VNCConn %p: I'm dead!"), this);
}



/*
  private members
*/


void VNCConn::on_stats_timer(wxTimerEvent& event)
{
  if(do_stats)
    {
      wxCriticalSectionLocker lock(mutex_stats);

      if(statistics.IsEmpty())
	statistics.Add(wxString()
		       + wxT("UTC time,") 
		       + wxT("conn time,")
		       + wxT("rcvd bytes,")
		       + wxT("rcvd bytes inflated,")
		       + wxT("upd count,")
		       + wxT("latency,")
		       + wxT("nack rate,")
		       + wxT("loss rate,")
		       + wxT("buf size,")
		       + wxT("buf fill,"));
      
      wxString sample;
      
      sample += (wxString() << (int)wxGetUTCTime()); // global UTC time
      sample += wxT(",");
      sample += (wxString() << (int)conn_stopwatch.Time()); // connection time
      sample += wxT(",");
      sample += (wxString() << upd_bytes); // rcvd bytes sampling
      sample += wxT(",");
      sample += (wxString() << upd_bytes_inflated); // rcvd bytes inflated sampling
      sample += wxT(",");
      sample += (wxString() << upd_count);  // number of updates sampling
      sample += wxT(",");
      sample += (wxString() << latency); // latency sampling
      sample += wxT(",");
      wxString nackrate_str = wxString::Format(wxT("%.4f"), getMCNACKedRatio());
      nackrate_str.Replace(wxT(","), wxT("."));
      sample += nackrate_str;            // nack rate sampling
      sample += wxT(",");
      wxString lossrate_str = wxString::Format(wxT("%.4f"), getMCLossRatio());
      lossrate_str.Replace(wxT(","), wxT("."));
      sample += lossrate_str;            // loss rate sampling
      sample += wxT(",");
      sample += (wxString() << getMCBufSize());  // buffer size sampling
      sample += wxT(",");
      sample += (wxString() << getMCBufFill());  // buffer fill sampling

      // add the sample
      statistics.Add(sample);
		
      // reset these
      upd_bytes = 0;
      upd_bytes_inflated = 0;
      upd_count = 0;
      latency = -1;


      latency_test_trigger = true;
    }
}



rfbBool VNCConn::thread_alloc_framebuffer(rfbClient* client)
{
  // get VNCConn object belonging to this client
  VNCConn* conn = (VNCConn*) rfbClientGetClientData(client, VNCCONN_OBJ_ID); 

  wxLogDebug(wxT("VNCConn %p: alloc'ing framebuffer w:%i, h:%i"), conn, client->width, client->height);

  // assert 32bpp, as requested with GetClient() in Init()
  if(client->format.bitsPerPixel != 32)
    {
      conn->err.Printf(_("Failure setting up framebuffer: wrong BPP!"));
      return false;
    }

  // ensure that we get the whole framebuffer in case of a resize!
  client->updateRect.x = client->updateRect.y = 0;
  client->updateRect.w = client->width; client->updateRect.h = client->height;

  // free
  if(client->frameBuffer)
    free(client->frameBuffer);

  // alloc, zeroed
  client->frameBuffer = (uint8_t*)calloc(1, client->width*client->height*client->format.bitsPerPixel/8);

  // notify our parent
  conn->thread_post_fbresize_notify();
 
  return client->frameBuffer ? TRUE : FALSE;
}




wxThread::ExitCode VNCConn::Entry()
{
 // init connection before going into main loop if this is not a listening one
 if (!thread_listenmode) {
     if (!ssh_host.IsEmpty()) {
         // ssh-tunneling, check whether it's password- or key-based
         if (ssh_password.IsOk()) {
             // password-based
             ssh_tunnel = ssh_tunnel_open_with_password(ssh_host.mb_str(),
                                                        wxAtoi(ssh_port),
                                                        ssh_username.mb_str(),
                                                        ssh_password.GetAsString().mb_str(),
                                                        cl->serverHost,
                                                        cl->serverPort,
                                                        this,
                                                        thread_on_ssh_fingerprint_check,
                                                        thread_on_ssh_error);
         } else {
             // key-based
             std::vector<char> ssh_priv_key;
             if (!ssh_priv_key_filename.IsEmpty()) {
                 wxFile sshPrivKeyFile(ssh_priv_key_filename);
                 if (sshPrivKeyFile.IsOpened()) {
                     wxFileOffset fileSize = sshPrivKeyFile.Length();
                     ssh_priv_key.resize(fileSize);
                     sshPrivKeyFile.Read(ssh_priv_key.data(), fileSize);
                 }
             }
             ssh_tunnel = ssh_tunnel_open_with_privkey(ssh_host.mb_str(),
                                                       wxAtoi(ssh_port),
                                                       ssh_username.mb_str(),
                                                       ssh_priv_key.data(),
                                                       ssh_priv_key.size(),
                                                       ssh_priv_key_password.GetAsString().mb_str(),
                                                       cl->serverHost,
                                                       cl->serverPort,
                                                       this,
                                                       thread_on_ssh_fingerprint_check,
                                                       thread_on_ssh_error);
         }

         if(ssh_tunnel) {
             cl->serverHost = strdup("127.0.0.1");
             cl->serverPort = ssh_tunnel_get_port(ssh_tunnel);
         } else {
             err.Printf(_("Failure connecting to SSH server at %s:%s!"), ssh_host, ssh_port);
             if(thread_shutdown) {
                 wxLogDebug("VNCConn %p: ssh_tunnel_open_* canceled.", this);
                 thread_post_init_notify(InitState::CONNECT_CANCEL);
             } else {
                 wxLogDebug("VNCConn %p: ssh_tunnel_open_* failed.", this);
                 thread_post_init_notify(InitState::CONNECT_FAIL);
             }
             wxLogDebug("VNCConn %p: vncthread done early", this);
             return 0;
         }
     }

     rfbClientLog("About to connect to '%s', port %d\n", cl->serverHost, cl->serverPort);

     // save these for the error case
     wxString host = wxString(cl->serverHost);
     int port = cl->serverPort;

     if (!rfbClientConnect(cl)) {
         err.Printf(_("Failure connecting to server at %s:%d!"), host, port);
         if(thread_shutdown) {
             wxLogDebug("VNCConn %p: rfbClientConnect() canceled.", this);
             thread_post_init_notify(InitState::CONNECT_CANCEL);
         } else {
             wxLogDebug("VNCConn %p: rfbClientConnect() failed.", this);
             thread_post_init_notify(InitState::CONNECT_FAIL);
         }
         wxLogDebug("VNCConn %p: vncthread done early", this);
         return 0;
     } else {
         wxLogDebug("VNCConn %p: rfbClientConnect() succeeded.", this);
         thread_post_init_notify(InitState::CONNECT_SUCCESS);
         if (!rfbClientInitialise(cl)) {
             err.Printf(_("Failure connecting to server at %s:%d!"), host, port);
             if(thread_shutdown) {
                 wxLogDebug("VNCConn %p: rfbClientInitialise() canceled.", this);
                 thread_post_init_notify(InitState::INITIALISE_CANCEL);
             } else {
                 wxLogDebug("VNCConn %p: rfbClientInitialise() failed.", this);
                 thread_post_init_notify(InitState::INITIALISE_FAIL);
             }
             wxLogDebug("VNCConn %p: vncthread done early", this);
             return 0;
         } else {
             wxLogDebug("VNCConn %p: rfbClientInitialise() succeeded.", this);

             // set the client sock to blocking again until libvncclient is fixed
#ifdef WIN32
             unsigned long block = 0;
             if (ioctlsocket(cl->sock, FIONBIO, &block) == SOCKET_ERROR) {
                 errno = WSAGetLastError();
#else
             int flags = fcntl(cl->sock, F_GETFL);
             if (flags < 0 || fcntl(cl->sock, F_SETFL, flags & ~O_NONBLOCK) < 0) {
#endif
                  rfbClientErr("Setting socket to blocking failed: %s\n", strerror(errno));
             }

             thread_post_init_notify(InitState::INITIALISE_SUCCESS);
         }
     }
  }

  int i=0;

  pointerEvent pe;
  keyEvent ke = {0, 0};

  bool listen_outcome_posted = false;

  while(! GetThread()->TestDestroy()) 
    {
      if(thread_listenmode)
	{
	  i=listenForIncomingConnectionsNoFork(cl, 100000); // 100 ms
          if (i == 0) {
	      // just notify about success once
	      if (!listen_outcome_posted) {
		  thread_post_listen_notify(0);
		  listen_outcome_posted = true;
	      }
          }
          if(i<0)
	    {
	      if(errno==EINTR)
		continue;
	      thread_post_listen_notify(1); //TODO add more error codes
              err.Printf(_("Failure listening on port %d!"), cl->listenPort);
              wxLogDebug("VNCConn %p: vncthread done w/ VNCConnListenNOTIFY(fail)", this);
              return 0;
	    }
	  if(i)
	    {
              // have this here in case of immediate connection
	      // but just notify about success once
	      if (!listen_outcome_posted) {
		  thread_post_listen_notify(0);
		  listen_outcome_posted = true;
	      }
	      thread_post_incomingconnection_notify();
	      break;
	    }
	}
      else
	{
	  // userinput replay here
	  {
	    wxCriticalSectionLocker lock(mutex_recordreplay);

	    if(replaying)
	      {
		if(userinput_pos < userinput.GetCount()) // still recorded input there
		  {
		    wxString ui_now = userinput[userinput_pos];
		    // get timestamp and strip it from string
		    long ts = wxAtol(ui_now.BeforeFirst(','));
		    ui_now = ui_now.AfterFirst(',');

		    if(ts <= recordreplay_stopwatch.Time()) // past or now, process it
		      {
			// get type
			wxString type = ui_now.BeforeFirst(',');
			ui_now = ui_now.AfterFirst(',');

			if(type == wxT("p"))
			  {
			    // get pointer x,y, buttmask
			    int x = wxAtoi(ui_now.BeforeFirst(','));
			    ui_now = ui_now.AfterFirst(',');
			    int y = wxAtoi(ui_now.BeforeFirst(','));
			    ui_now = ui_now.AfterFirst(',');
			    int bmask = wxAtoi(ui_now);

			    // and send
			    SendPointerEvent(cl, x, y, bmask);
			  }

			if(type == wxT("k"))
			  {
			    // get keysym
			    rfbKeySym keysym = wxAtoi(ui_now.BeforeFirst(','));
			    ui_now = ui_now.AfterFirst(',');
			    bool down = wxAtoi(ui_now); 

			    // and send 
			    SendKeyEvent(cl, keysym, down);
			  }

			// advance to next input
			++userinput_pos;
		      }
		  }
		else if (replay_loop)
		  {
		    userinput_pos = 0; // rewind
		    recordreplay_stopwatch.Start(); // restart
		  }
		else
		  {
		    replaying = false; // all done
		    thread_post_replayfinished_notify();
		  }

	      }
	  }

	  // send everything that's inside the input queues
	  while(pointer_event_q.ReceiveTimeout(0, pe) != wxMSGQUEUE_TIMEOUT) // timeout == empty
	    thread_send_pointer_event(pe);
	  while(key_event_q.ReceiveTimeout(0, ke) != wxMSGQUEUE_TIMEOUT) // timeout == empty
	    thread_send_key_event(ke);

	  {
	    wxCriticalSectionLocker lock(mutex_stats);
	    if(latency_test_trigger)
	      {
		latency_test_trigger = false;
		thread_send_latency_probe();
	      }
	  }

          // do FastRequest when interval set and expired and only after one full update was received
	  if(fastrequest_interval && (size_t)fastrequest_stopwatch.Time() > fastrequest_interval && upd_count)
	    {
	      if(isMulticast())
		SendMulticastFramebufferUpdateRequest(cl, TRUE);
	      else
		SendFramebufferUpdateRequest(cl, 0, 0, cl->width, cl->height, TRUE);

	      fastrequest_stopwatch.Start(); // restart
	    }


          // request update and handle response
          {
          wxCriticalSectionLocker lock(mutex_framebuffer);
          if(!rfbProcessServerMessage(cl, 500))
	    {
	      if(errno == EINTR)
		continue;
	      wxLogDebug(wxT("VNCConn %p: vncthread rfbProcessServerMessage() failed"), this);
              if(thread_shutdown) {
                  // it can (rarely) happen that the socket close in Shutdown() makes rfbProcessServerMessage() fail, too
                  thread_post_disconnect_notify(0);
                  wxLogDebug("VNCConn %p: vncthread done w/ VNCConnDisconnectNOTIFY(by-local)", this);
              } else {
                  thread_post_disconnect_notify(1);
                  wxLogDebug("VNCConn %p: vncthread done w/ VNCConnDisconnectNOTIFY(by-remote)", this);
              }
              return 0;
	    }
          }

	  /*
	    Compute nacked/loss ratio: We take a ratio sample every second and put it into a sample queue
	    of size N. Action is taken when the average sample value of the whole buffer exceeds a per-action
	    limit. This has advantages over taking a sample every N seconds: First, it's able to catch say a 5sec burst
	    that could be missed by two adjacent 10sec samples (one catches 2sec, the next one 3sec - no action triggered
	    although condition present). Second, this way we're able to show a value to the user every second independent
	    of the sample time frame.
	  */
   	  if(isMulticast() && multicastratio_stopwatch.Time() >= 1000)
	    {
	      // restart
	      multicastratio_stopwatch.Start();
  
	      /*
		take sample
	      */
	      {
		// the fifos are read by the GUI thread as well!
		wxCriticalSectionLocker lock(mutex_multicastratio);
		
		if(multicastNACKedRatios.size() >= MULTICAST_RATIO_SAMPLES) // make room if size exceeded
		  multicastNACKedRatios.pop_front();
		if(cl->multicastPktsRcvd + cl->multicastPktsNACKed > 0)
		  multicastNACKedRatios.push_back(cl->multicastPktsNACKed/(double)(cl->multicastPktsRcvd + cl->multicastPktsNACKed));
		else
		  multicastNACKedRatios.push_back(-1); // nothing to measure, add invalid marker

		if(multicastLossRatios.size() >= MULTICAST_RATIO_SAMPLES) // make room if size exceeded
		  multicastLossRatios.pop_front();
		if(cl->multicastPktsRcvd + cl->multicastPktsLost > 0)
		  multicastLossRatios.push_back(cl->multicastPktsLost/(double)(cl->multicastPktsRcvd + cl->multicastPktsLost));
		else
		  multicastLossRatios.push_back(-1); // nothing to measure, add invalid marker

		// reset the values we sample
		cl->multicastPktsRcvd = cl->multicastPktsNACKed = cl->multicastPktsLost = 0;
	      }

	      /*
		And act accordingly, but only after the ratio deques are at least half full.
		When a client joins a multicast group with heavy traffic going on, it will lose
		a lot of packets in the very beginning because there is a considerable time
		amount between it's multicast socket creation and the first read. Thus, the socket
		buffer is likely to overflow in this start situation, resulting in packet loss.
	      */
	      if(multicastLossRatios.size() >= MULTICAST_RATIO_SAMPLES/2)
		{
		  if(getMCLossRatio() > 0.5)
		    {
		      rfbClientLog("MultiVNC: loss ratio > 0.5, falling back to unicast\n");
		      wxLogDebug(wxT("VNCConn %p: multicast loss ratio > 0.5, falling back to unicast"), this);
		      cl->multicastDisabled = TRUE;
		      SendFramebufferUpdateRequest(cl, 0, 0, cl->width, cl->height, FALSE);
		    }
		  else if(getMCLossRatio() > 0.2)
		    {
		      rfbClientLog("MultiVNC: loss ratio > 0.2, requesting a full multicast framebuffer update\n");
		      SendMulticastFramebufferUpdateRequest(cl, FALSE);
		      cl->multicastPktsLost /= 2;
		    }
		}
	    }



	  int now = isMulticast();
	  if(now != multicastStatus)
	    {
	      multicastStatus = now;
	      thread_post_unimultichanged_notify();
	    }
	}
    }

  if(thread_shutdown) {
      // happens when TestDestroy() makes the loop end
      thread_post_disconnect_notify(0);
      wxLogDebug("VNCConn %p: vncthread done w/ VNCConnDisconnectNOTIFY(by-local)", this);
  } else {
      // happens when listening socket got a connection
      wxLogDebug("VNCConn %p: vncthread done", this);
  }
  return 0;
}





bool VNCConn::thread_send_pointer_event(pointerEvent &event)
{
  // first, handle cuttext sending, if any
  if(event.entering && !cuttext.IsEmpty())
    {
      wxCriticalSectionLocker lock(mutex_cuttext); // since cuttext can be set from the main thread

      // first, try sending UTF-8
      if(SendClientCutTextUTF8(cl, const_cast<char*>(cuttext.utf8_str().data()), strlen(cuttext.utf8_str().data()))) {
          wxLogDebug(wxT("VNCConn %p: sent UTF-8 cuttext: '%s'"), this, cuttext.utf8_str());
      } else {
          // server does not support Extended Clipboard, try Latin-1.
          // if encoding fails, length() returns 0
          if(cuttext.mb_str(wxCSConv("iso-8859-1")).length())
              {
                  char* encoded_text = strdup(cuttext.mb_str(wxCSConv(wxT("iso-8859-1"))));
                  SendClientCutText(cl, encoded_text, strlen(encoded_text));
                  wxLogDebug(wxT("VNCConn %p: sent Latin-1 cuttext: '%s'"), this, encoded_text);
                  free(encoded_text);
              }
          else
              wxLogDebug(wxT("VNCConn %p: sending Latin-1 cuttext FAILED, could not convert '%s' to ISO-8859-1"), this, cuttext.c_str());
      }
    }


  // record here
  {
    wxCriticalSectionLocker lock(mutex_recordreplay);

    if(recording)
      {
	wxString ui_now;
	ui_now += (wxString() << (int)recordreplay_stopwatch.Time()); 
	ui_now += wxT(",");
	ui_now += wxT("p"); // is pointer event
	ui_now += wxT(",");
	ui_now += (wxString() << event.x);
	ui_now += wxT(",");
	ui_now += (wxString() << event.y);
	ui_now += wxT(",");
	ui_now += (wxString() << event.buttonmask);

	userinput.Add(ui_now);
      }
  }

  wxLogDebug(wxT("VNCConn %p: sending pointer event at (%d,%d), buttonmask %d"), this, event.x, event.y, event.buttonmask);
  return SendPointerEvent(cl, event.x, event.y, event.buttonmask);
}




bool VNCConn::thread_send_key_event(keyEvent &event)
{
  // record here
  {
    wxCriticalSectionLocker lock(mutex_recordreplay);

    if(recording)
      {
	wxString ui_now;
	ui_now += (wxString() << (int)recordreplay_stopwatch.Time()); 
	ui_now += wxT(",");
	ui_now += wxT("k"); // is key event
	ui_now += wxT(",");
	ui_now += (wxString() << event.keysym);
	ui_now += wxT(",");
	ui_now += (wxString() << event.down);

	userinput.Add(ui_now);
      }
  }

  return SendKeyEvent(cl, event.keysym, event.down);
}



bool VNCConn::thread_send_latency_probe()
{
  bool result = TRUE;
  // latency check start
  if(SupportsClient2Server(cl, rfbXvp)) // favor xvp over the rect check 
    {
      if(!latency_test_xvpmsg_sent)
	{
	  result = SendXvpMsg(cl, LATENCY_TEST_XVP_VER, 2);
	  latency_test_xvpmsg_sent = true;
	  latency_stopwatch.Start();
	  wxLogDebug(wxT("VNCConn %p: xvp message sent to test latency"), this);
	}
    }
  else  // check using special rect
    {
      if(!latency_test_rect_sent)
	{
	  result = SendFramebufferUpdateRequest(cl, LATENCY_TEST_RECT, FALSE);
	  latency_test_rect_sent = true;
	  latency_stopwatch.Start();
	  wxLogDebug(wxT("VNCConn %p: fb update request sent to test latency"), this);
	}
    }
  return result;
}

void VNCConn::thread_post_listen_notify(int error) {
  wxLogDebug(wxT("VNCConn %p: post_listen_notify(%d)"), this, error);
  wxCommandEvent event(VNCConnListenNOTIFY, wxID_ANY);
  event.SetEventObject(this); // set sender
  event.SetInt(error);
  wxPostEvent((wxEvtHandler*)parent, event);
}

void VNCConn::thread_post_init_notify(int state) {
  wxString stateString;
  switch(state) {
  case InitState::CONNECT_SUCCESS:
      stateString = "InitState::CONNECT_SUCCESS";
      break;
  case InitState::CONNECT_FAIL:
      stateString = "InitState::CONNECT_FAIL";
      break;
  case InitState::CONNECT_CANCEL:
      stateString = "InitState::CONNECT_CANCEL";
      break;
  case InitState::INITIALISE_SUCCESS:
      stateString = "InitState::INITIALISE_SUCCESS";
      break;
  case InitState::INITIALISE_FAIL:
      stateString = "InitState::INITIALISE_FAIL";
      break;
  case InitState::INITIALISE_CANCEL:
      stateString = "InitState::INITIALISE_CANCEL";
      break;
  }
  wxLogDebug(wxT("VNCConn %p: post_init_notify(%s)"), this, stateString);
  wxCommandEvent event(VNCConnInitNOTIFY, wxID_ANY);
  event.SetEventObject(this); // set sender
  event.SetInt(state);
  wxPostEvent((wxEvtHandler*)parent, event);
}

void VNCConn::thread_post_getpasswd_notify() {
  wxLogDebug(wxT("VNCConn %p: post_getpasswd_notify()"), this);
  wxCommandEvent event(VNCConnGetPasswordNOTIFY, wxID_ANY);
  event.SetEventObject(this); // set sender
  wxPostEvent((wxEvtHandler*)parent, event);
}

void VNCConn::thread_post_getcreds_notify(bool withUserPrompt) {
  wxLogDebug(wxT("VNCConn %p: post_getcreds_notify()"), this);
  wxCommandEvent event(VNCConnGetCredentialsNOTIFY, wxID_ANY);
  event.SetEventObject(this); // set sender
  event.SetInt(withUserPrompt);
  wxPostEvent((wxEvtHandler*)parent, event);
}


void VNCConn::thread_post_incomingconnection_notify() 
{
  wxLogDebug(wxT("VNCConn %p: post_incomingconnection_notify()"), this);

  // new NOTIFY event, we got no window id
  wxCommandEvent event(VNCConnIncomingConnectionNOTIFY, wxID_ANY);
  event.SetEventObject(this); // set sender

  // Send it
  wxPostEvent((wxEvtHandler*)parent, event);
}


void VNCConn::thread_post_disconnect_notify(int reason)
{
  wxLogDebug(wxT("VNCConn %p: post_disconnect_notify(%d)"), this, reason);

  // new NOTIFY event, we got no window id
  wxCommandEvent event(VNCConnDisconnectNOTIFY, wxID_ANY);
  event.SetEventObject(this); // set sender
  event.SetInt(reason);
  // Send it
  wxPostEvent((wxEvtHandler*)parent, event);
}


void VNCConn::thread_post_update_notify(int x, int y, int w, int h)
{
  VNCConnUpdateNotifyEvent event(VNCConnUpdateNOTIFY, wxID_ANY);
  event.SetEventObject(this); // set sender

  // set info about what was updated
  event.rect = wxRect(x, y, w, h);
  wxLogDebug(wxT("VNCConn %p: post_update_notify(%i,%i,%i,%i)"), this,
	     event.rect.x,
	     event.rect.y,
	     event.rect.width,
	     event.rect.height);
  
  // Send it
  wxPostEvent((wxEvtHandler*)parent, event);
}


void VNCConn::thread_post_fbresize_notify() 
{
  wxLogDebug(wxT("VNCConn %p: post_fbresize_notify() (%i, %i)"), 
	     this,
	     getFrameBufferWidth(),
	     getFrameBufferHeight());

  // new NOTIFY event, we got no window id
  wxCommandEvent event(VNCConnFBResizeNOTIFY, wxID_ANY);
  event.SetEventObject(this); // set sender

  // Send it
  wxPostEvent((wxEvtHandler*)parent, event);
}



void VNCConn::thread_post_cuttext_notify()
{
  // new NOTIFY event, we got no window id
  wxCommandEvent event(VNCConnCuttextNOTIFY, wxID_ANY);
  event.SetEventObject(this); // set sender

  // Send it
  wxPostEvent((wxEvtHandler*)parent, event);
}



void VNCConn::thread_post_bell_notify()
{
  // new NOTIFY event, we got no window id
  wxCommandEvent event(VNCConnBellNOTIFY, wxID_ANY);
  event.SetEventObject(this); // set sender

  // Send it
  wxPostEvent((wxEvtHandler*)parent, event);
}



void VNCConn::thread_post_unimultichanged_notify()
{
  wxCommandEvent event(VNCConnUniMultiChangedNOTIFY, wxID_ANY);
  event.SetEventObject(this); // set sender
  
  // Send it
  wxPostEvent((wxEvtHandler*)parent, event);
}


void VNCConn::thread_post_replayfinished_notify()
{
  wxCommandEvent event(VNCConnReplayFinishedNOTIFY, wxID_ANY);
  event.SetEventObject(this); // set sender
  
  // Send it
  wxPostEvent((wxEvtHandler*)parent, event);
}


char* VNCConn::thread_getpasswd(rfbClient *client) {
    VNCConn* conn = (VNCConn*) rfbClientGetClientData(client, VNCCONN_OBJ_ID);

    conn->require_auth = true;

    if (!conn->getPassword().IsOk()) {
        // get password from user
	conn->thread_post_getpasswd_notify();
	// wxMutexes are not recursive under Unix, so test first
        if (conn->mutex_auth.TryLock() == wxMUTEX_NO_ERROR) {
            conn->mutex_auth.Lock();
        }
	wxLogDebug("VNCConn %p: vncthread waiting for password", conn);
        conn->condition_auth.Wait();
	wxLogDebug("VNCConn %p: vncthread done waiting for password", conn);
	// we get here once setPassword() was called
    }
    return strdup(conn->getPassword().GetAsString().char_str());
};


rfbCredential* VNCConn::thread_getcreds(rfbClient *client, int type) {
    VNCConn *conn = VNCConn::getVNCConnFromRfbClient(client);

    conn->require_auth = true;

    if(type == rfbCredentialTypeUser) {

	if(conn->getUserName().IsEmpty()
            || !conn->getPassword().IsOk()) {
	    // username and/or password needed
            conn->thread_post_getcreds_notify(conn->getUserName().IsEmpty());
            // wxMutexes are not recursive under Unix, so test first
            if (conn->mutex_auth.TryLock() == wxMUTEX_NO_ERROR) {
                conn->mutex_auth.Lock();
            }
            wxLogDebug("VNCConn %p: vncthread waiting for credentials", conn);
            conn->condition_auth.Wait();
            wxLogDebug("VNCConn %p: vncthread done waiting for credentials",
                       conn);
            // we get here once setPassword() was called
        }

        rfbCredential *c = (rfbCredential *)calloc(1, sizeof(rfbCredential));
        c->userCredential.username = strdup(conn->getUserName().char_str());
        c->userCredential.password = strdup(conn->getPassword().GetAsString().char_str());
        return c;
    }

    return NULL;
};



void VNCConn::thread_got_update(rfbClient* client,int x,int y,int w,int h)
{
  VNCConn* conn = (VNCConn*) rfbClientGetClientData(client, VNCCONN_OBJ_ID); 
  if(! conn->GetThread()->TestDestroy())
    {
      conn->updated_rect.Union(wxRect(x, y, w, h));

      if(conn->do_stats)
	{
	  wxCriticalSectionLocker lock(conn->mutex_stats);

	  wxRect this_update_rect = wxRect(x,y,w,h);

	  // compressed bytes
	  conn->upd_bytes += conn->cl->bytesRcvd;
	  conn->upd_bytes += conn->cl->multicastBytesRcvd;
	  conn->cl->bytesRcvd = conn->cl->multicastBytesRcvd = 0;

	  // uncompressed bytes
	  conn->upd_bytes_inflated += w*h*BYTESPERPIXEL;

	  // latency check, rect case
	  if(conn->latency_test_rect_sent && this_update_rect.Contains(wxRect(LATENCY_TEST_RECT)))
	    {
	      conn->latency_stopwatch.Pause();
	      conn->latency = conn->latency_stopwatch.Time();
	      conn->latency_test_rect_sent = false;

	      wxLogDebug(wxT("VNCConn %p: got update containing latency test rect, took %ims"), conn, conn->latency_stopwatch.Time());
	    }
	}
    }
}




void VNCConn::thread_update_finished(rfbClient* client)
{
  VNCConn* conn = (VNCConn*) rfbClientGetClientData(client, VNCCONN_OBJ_ID);
  if(! conn->GetThread()->TestDestroy())
    {
      if(!conn->updated_rect.IsEmpty())
	conn->thread_post_update_notify(conn->updated_rect.x, conn->updated_rect.y, conn->updated_rect.width, conn->updated_rect.height);

      conn->updated_rect = wxRect();

      if(conn->do_stats || conn->fastrequest_interval)
	{
	  wxCriticalSectionLocker lock(conn->mutex_stats);
	  conn->upd_count++;
	}
    }
}



void VNCConn::thread_kbd_leds(rfbClient* cl, int value, int pad)
{
  VNCConn* conn = (VNCConn*) rfbClientGetClientData(cl, VNCCONN_OBJ_ID); 
  wxLogDebug(wxT("VNCConn %p: Led State= 0x%02X"), conn, value);
}


void VNCConn::thread_textchat(rfbClient* cl, int value, char *text)
{
  VNCConn* conn = (VNCConn*) rfbClientGetClientData(cl, VNCCONN_OBJ_ID); 
  switch(value)
    {
    case (int)rfbTextChatOpen:
      wxLogDebug(wxT("VNCConn %p: got textchat open\n"), conn);
      break;
    case (int)rfbTextChatClose:
      wxLogDebug(wxT("VNCConn %p: got textchat close\n"), conn);
      break;
    case (int)rfbTextChatFinished:
      wxLogDebug(wxT("VNCConn %p: got textchat finish\n"), conn);
      break;
    default:
      wxLogDebug(wxT("VNCConn %p: got textchat text: '%s'\n"), conn, text);
    }
}


void VNCConn::thread_got_cuttext(rfbClient *cl, const char *text, int len)
{
  VNCConn* conn = (VNCConn*) rfbClientGetClientData(cl, VNCCONN_OBJ_ID);

  wxLogDebug(wxT("VNCConn %p: got Latin1 cuttext: '%s'"), conn, wxString(text, wxCSConv(wxT("iso-8859-1"))).c_str());

  wxCriticalSectionLocker lock(conn->mutex_cuttext); // since cuttext can also be set from the main thread
  conn->cuttext = wxString(text, wxCSConv(wxT("iso-8859-1")));
  conn->thread_post_cuttext_notify();
}


void VNCConn::thread_got_cuttext_utf8(rfbClient *cl, const char *text, int len)
{
  VNCConn* conn = (VNCConn*) rfbClientGetClientData(cl, VNCCONN_OBJ_ID);

  wxLogDebug(wxT("VNCConn %p: got UTF-8 cuttext: '%s'"), conn, wxString(text, wxConvUTF8).c_str());

  wxCriticalSectionLocker lock(conn->mutex_cuttext); // since cuttext can also be set from the main thread
  conn->cuttext = wxString(text, wxConvUTF8);
  conn->thread_post_cuttext_notify();
}


void VNCConn::thread_bell(rfbClient *cl)
{
  VNCConn* conn = (VNCConn*) rfbClientGetClientData(cl, VNCCONN_OBJ_ID);
  wxLogDebug(wxT("VNCConn %p: bell"), conn);
  conn->thread_post_bell_notify();
}


void VNCConn::thread_handle_xvp(rfbClient *cl, uint8_t ver, uint8_t code)
{
  VNCConn* conn = (VNCConn*) rfbClientGetClientData(cl, VNCCONN_OBJ_ID);
  wxLogDebug(wxT("VNCConn %p: handling xvp msg version %d code %d"), conn, ver, code);

  if(conn->latency_test_xvpmsg_sent && ver == LATENCY_TEST_XVP_VER && code == rfbXvp_Fail) 
    {
      wxCriticalSectionLocker lock(conn->mutex_stats);
      conn->latency_stopwatch.Pause();
      conn->latency = conn->latency_stopwatch.Time();
      conn->latency_test_xvpmsg_sent = false;
      
      wxLogDebug(wxT("VNCConn %p: got latency test xvp message back, took %ims"), conn, conn->latency_stopwatch.Time());
    }
}




// there's no per-connection log since we cannot find out which client
// called the logger function :-(
wxArrayString VNCConn::log;
wxCriticalSection VNCConn::mutex_log;
bool VNCConn::do_logfile;

void VNCConn::thread_logger(const char *format, ...)
{
  if(!rfbEnableClientLogging)
    return;

  // since we're accessing some global things here from different threads
  wxCriticalSectionLocker lock(mutex_log);

  wxChar timebuf[256];
  time_t log_clock;
  time(&log_clock);
  wxStrftime(timebuf, WXSIZEOF(timebuf), _T("%d/%m/%Y %X "), localtime(&log_clock));

  // global log string array
  va_list args;  
  wxString wx_format(format, wxConvUTF8);
  va_start(args, format);
  char msg[1024];
  vsnprintf(msg, 1024, format, args);
  log.Add( wxString(timebuf) + wxString(msg, wxConvUTF8));
  va_end(args);

  // global log file
  if(do_logfile)
    {
      FILE* logfile;
      wxString logfile_str = LOGFILE;
      // delete logfile on program startup
      static bool firstrun = 1;
      if(firstrun)
	{
	  remove(logfile_str.char_str());
	  firstrun = 0;
	}
      
      logfile=fopen(logfile_str.char_str(),"a");    
      
      va_start(args, format);
      fprintf(logfile, "%s", (const char*)wxString(timebuf).mb_str());
      vfprintf(logfile, format, args);
      va_end(args);

      fclose(logfile);
    }
 
  // and stderr
  va_start(args, format);
  fprintf(stderr, "%s", (const char*)wxString(timebuf).mb_str());
  vfprintf(stderr, format, args);
  va_end(args);
}


void VNCConn::thread_on_ssh_error(void *client, ssh_tunnel_error_t error_code,  const char *error_message) {
    wxString msg = wxString::Format("SSH tunnel error: %d - %s",  error_code, error_message);
    rfbClientErr(msg.mb_str());
    VNCConn* conn = (VNCConn*) client;
    conn->err.Printf(msg);
}


int VNCConn::thread_on_ssh_fingerprint_check(void *client,
                                             const char *fingerprint,
                                             int fingerprint_len,
                                             const char *host) {
    VNCConn* conn = (VNCConn*) client;
    // TODO
    return 0;
}


/*
  public members
*/

bool VNCConn::setupClient()
{
  wxLogDebug(wxT("VNCConn %p: setupClient()"), this);

  if(cl) // already set up
    {
      wxLogDebug(wxT("VNCConn %p: setupClient() already done"), this);
      return false;
    }

  // this takes (int bitsPerSample,int samplesPerPixel, int bytesPerPixel) 
  // 5,3,2 and 8,3,4 seem possible
  cl=rfbGetClient(BITSPERSAMPLE, SAMPLESPERPIXEL, BYTESPERPIXEL);
 
  rfbClientSetClientData(cl, VNCCONN_OBJ_ID, this); 

  // callbacks
  cl->MallocFrameBuffer = thread_alloc_framebuffer;
  cl->GotFrameBufferUpdate = thread_got_update;
  cl->FinishedFrameBufferUpdate = thread_update_finished;
  cl->GetPassword = thread_getpasswd;
  cl->GetCredential = thread_getcreds;
  cl->HandleKeyboardLedState = thread_kbd_leds;
  cl->HandleTextChat = thread_textchat;
  cl->GotXCutText = thread_got_cuttext;
  cl->GotXCutTextUTF8 = thread_got_cuttext_utf8;
  cl->Bell = thread_bell;
  cl->HandleXvpMsg = thread_handle_xvp;

  cl->canHandleNewFBSize = TRUE;
  cl->connectTimeout = 5;

  return true;
}



void VNCConn::Listen(int port)
{
  wxLogDebug(wxT("VNCConn %p: Listen() port %d"), this, port);

  if(!cl)
      setupClient();

  cl->listenPort = cl->listen6Port = port;

  thread_listenmode = true;

  if( CreateThread() != wxTHREAD_NO_ERROR )
    {
      err.Printf(_("Could not create VNC listener thread!"));
      Shutdown();
      thread_post_listen_notify(1); //TODO add more error codes
      return;
    }

  if( GetThread()->Run() != wxTHREAD_NO_ERROR )
    {
      err.Printf(_("Could not start VNC listener thread!")); 
      Shutdown();
      thread_post_listen_notify(1); //TODO add more error codes
      return;
    }
}


bool VNCConn::Init(const wxString& host,
                   int repeaterId,
                   const wxString& username,
		   const wxSecretValue& password,
                   const wxString& ssh_host,
                   int ssh_port,
                   const wxString& ssh_user,
                   const wxSecretValue& ssh_password,
                   const wxString& ssh_priv_key_filename,
                   const wxSecretValue& ssh_priv_key_password,
		   const wxString& encodings, int compresslevel, int quality, bool multicast, int multicast_socketrecvbuf, int multicast_recvbuf)
{
  wxLogDebug("VNCConn %p: Init() host '%s'", this, host);

  if(!cl)
      setupClient();

  if(cl->frameBuffer || (GetThread() && GetThread()->IsRunning()))
    {
      err.Printf(_("Connection already initialised. Please disconnect first."));
      return false;
    }

  // reset stats before doing new connection
  resetStats();

  cl->programName = "VNCConn";
  parseHostString(host.mb_str(), 5900, &cl->serverHost, &cl->serverPort);
  this->username = username;
  this->password = password;
  // Support short-form (:0, :1) 
  if(cl->serverPort < 100)
    cl->serverPort += 5900;

  if(repeaterId >= 0) {
      cl->destHost = strdup("ID");
      cl->destPort = repeaterId;
  }

  this->ssh_host = ssh_host;
  this->ssh_port = wxString::Format(wxT("%i"), ssh_port);
  this->ssh_username = ssh_user;
  this->ssh_password = ssh_password;
  this->ssh_priv_key_filename = ssh_priv_key_filename;
  this->ssh_priv_key_password = ssh_priv_key_password;

  cl->appData.compressLevel = compresslevel;
  cl->appData.qualityLevel = quality;
  cl->appData.encodingsString = strdup(encodings.mb_str());
  if(multicast)
    {
      cl->canHandleMulticastVNC = TRUE;
      cl->multicastSocketRcvBufSize = multicast_socketrecvbuf*1024;
      cl->multicastRcvBufSize = multicast_recvbuf*1024;
      multicastratio_stopwatch.Start();
    }
  else
    cl->canHandleMulticastVNC = FALSE;

  // this is like our main loop
  thread_listenmode = false;
  thread_shutdown = false;
  if( CreateThread() != wxTHREAD_NO_ERROR )
    {
      err.Printf(_("Could not create VNC thread!"));
      Shutdown();
      return false;
    }

  if( GetThread()->Run() != wxTHREAD_NO_ERROR )
    {
      err.Printf(_("Could not start VNC thread!")); 
      Shutdown();
      return false;
    }

  conn_stopwatch.Start();
  pointer_scroll_stopwatch.Start();

  return true;
}




void VNCConn::Shutdown()
{
  wxLogDebug(wxT("VNCConn %p: Shutdown()"), this);

  conn_stopwatch.Pause();

  thread_shutdown = true;

  mutex_auth.Unlock();

  if (ssh_tunnel) {
      wxLogDebug(wxT( "VNCConn %p: Shutdown() closing SSH tunnel"), this);
      ssh_tunnel_close(ssh_tunnel);
      ssh_tunnel = NULL;
  }

  // break any connection so that vnc thread moves over blocking calls
  if(cl) {
      wxLogDebug(wxT( "VNCConn %p: Shutdown() closing connection"), this);
      rfbCloseSocket(cl->sock);
  }

  // end vnc thread and wait for it to get done
  if(GetThread() && GetThread()->IsRunning())
    {
      wxLogDebug(wxT( "VNCConn %p: Shutdown() before vncthread delete"), this);

      GetThread()->Delete(); // this blocks if thread is joinable, i.e. on stack
      wxLogDebug(wxT("VNCConn %p: Shutdown() after vncthread delete"), this);
    }

  // then, clean up client
  if(cl) {
      wxLogDebug(wxT( "VNCConn %p: Shutdown() cleaning up client"), this);
      // this one was strdup'ed before
      if(!cl->listenSpecified)
	free((void*)cl->appData.encodingsString);

      // in case we called listen, canceled that, and now want to connect to some
      // host via Init()
      cl->listenSpecified = FALSE;

      if(cl->frameBuffer)
	{
	  free(cl->frameBuffer);
	  cl->frameBuffer = 0;
	}

      rfbClientCleanup(cl);
      cl = 0;
    }
}


void VNCConn::setFastRequest(size_t interval)
{
  fastrequest_interval = interval;
}

bool VNCConn::setDSCP(uint8_t dscp)
{
  if(cl && cl->sock >= 0)
    return SetDSCP(cl->sock, dscp);
  else
    return false;
}


// this simply posts the mouse event into the worker thread's input queue
void VNCConn::sendPointerEvent(wxMouseEvent &event)
{
  if(replaying)
    return;

  pointerEvent pev = {event.m_x, event.m_y, 0, event.Entering()};

  /*
    Populate buttonmask with pressed buttons
   */
  if(event.LeftIsDown())
    pev.buttonmask |= rfbButton1Mask;

  if(event.MiddleIsDown())
    pev.buttonmask |= rfbButton2Mask;

  if(event.RightIsDown())
    pev.buttonmask |= rfbButton3Mask;

  if (event.GetWheelRotation() != 0 && event.GetWheelAxis() == wxMOUSE_WHEEL_VERTICAL && pointer_scroll_stopwatch.Time() >= 20) {

      if(event.GetWheelRotation() > 0) {
          pev.buttonmask |= rfbWheelUpMask;
      }

      if(event.GetWheelRotation() < 0) {
          pev.buttonmask |= rfbWheelDownMask;
      }

      // restart the timer
      pointer_scroll_stopwatch.Start();
  }

  // Queue event
  if(GetThread() && GetThread()->IsRunning())
    pointer_event_q.Post(pev);

  // For each queued button 4 or 5 down, queue another synthesised button 4 or 5 up
  if (pev.buttonmask & rfbWheelUpMask || pev.buttonmask & rfbWheelDownMask) {

      if (pev.buttonmask & rfbWheelUpMask) {
          pev.buttonmask ^= rfbWheelUpMask;
      }

      if (pev.buttonmask & rfbWheelDownMask) {
          pev.buttonmask ^= rfbWheelDownMask;
      }

      if(GetThread() && GetThread()->IsRunning())
          pointer_event_q.Post(pev);
  }

}


// because of the possible wxKeyEvent.Skip(), this posts the found keysym + down
bool VNCConn::sendKeyEvent(wxKeyEvent &event, bool down, bool isChar)
{
  if(replaying)
    return false;

  keyEvent kev = {0,0};

  if(isChar)
    {
      wxLogDebug(wxT("VNCConn %p: got CHAR key event"), this);
      wxLogDebug(wxT("VNCConn %p:     wxkeycode:      %d"), this, event.GetKeyCode());
      wxLogDebug(wxT("VNCConn %p:     wxkeycode char: %c"), this, event.GetKeyCode());
      wxLogDebug(wxT("VNCConn %p:     unicode:        %d"), this, event.GetUnicodeKey());
      wxLogDebug(wxT("VNCConn %p:     unicode char:   %c"), this, event.GetUnicodeKey()); 

      // get tranlated char
      kev.keysym = event.GetKeyCode();
      // if we got ne keycode, try unicodekey
      if(kev.keysym==0) 
	kev.keysym = event.GetUnicodeKey();

      // if wxwidgets translates a key combination into a
      // value below 32, revert this here.
      // we dont't send ASCII 0x03, but ctrl and then a 'c'!
      if(kev.keysym <= 32)
	{
	  kev.keysym += 96;
	  wxLogDebug(wxT("VNCConn %p: translating key to: %d"), this, kev.keysym);
	}

      wxLogDebug(wxT("VNCConn %p: sending rfbkeysym: 0x%.3x down"), this, kev.keysym);
      wxLogDebug(wxT("VNCConn %p: sending rfbkeysym: 0x%.3x  up"), this, kev.keysym);
      
      // down, then up
      if(GetThread() && GetThread()->IsRunning())
	{
	  kev.down = true;
	  key_event_q.Post(kev);
	  kev.down = false;
	  key_event_q.Post(kev);
	}
      return true;
    }
  else
    {
      // lookup keysym  
      switch(event.GetKeyCode()) 
	{
	case WXK_BACK: kev.keysym = XK_BackSpace; break;
	case WXK_TAB: kev.keysym = XK_Tab; break;
	case WXK_CLEAR: kev.keysym = XK_Clear; break;
	case WXK_RETURN: kev.keysym = XK_Return; break;
	case WXK_PAUSE: kev.keysym = XK_Pause; break;
	case WXK_ESCAPE: kev.keysym = XK_Escape; break;
	case WXK_SPACE: kev.keysym = XK_space; break;
	case WXK_DELETE: kev.keysym = XK_Delete; break;
	case WXK_NUMPAD0: kev.keysym = XK_KP_0; break;
	case WXK_NUMPAD1: kev.keysym = XK_KP_1; break;
	case WXK_NUMPAD2: kev.keysym = XK_KP_2; break;
	case WXK_NUMPAD3: kev.keysym = XK_KP_3; break;
	case WXK_NUMPAD4: kev.keysym = XK_KP_4; break;
	case WXK_NUMPAD5: kev.keysym = XK_KP_5; break;
	case WXK_NUMPAD6: kev.keysym = XK_KP_6; break;
	case WXK_NUMPAD7: kev.keysym = XK_KP_7; break;
	case WXK_NUMPAD8: kev.keysym = XK_KP_8; break;
	case WXK_NUMPAD9: kev.keysym = XK_KP_9; break;
	case WXK_NUMPAD_DECIMAL: kev.keysym = XK_KP_Decimal; break;
	case WXK_NUMPAD_DIVIDE: kev.keysym = XK_KP_Divide; break;
	case WXK_NUMPAD_MULTIPLY: kev.keysym = XK_KP_Multiply; break;
	case WXK_NUMPAD_SUBTRACT: kev.keysym = XK_KP_Subtract; break;
	case WXK_NUMPAD_ADD: kev.keysym = XK_KP_Add; break;
	case WXK_NUMPAD_ENTER: kev.keysym = XK_KP_Enter; break;
	case WXK_NUMPAD_EQUAL: kev.keysym = XK_KP_Equal; break;
	case WXK_UP: kev.keysym = XK_Up; break;
	case WXK_DOWN: kev.keysym = XK_Down; break;
	case WXK_RIGHT: kev.keysym = XK_Right; break;
	case WXK_LEFT: kev.keysym = XK_Left; break;
	case WXK_INSERT: kev.keysym = XK_Insert; break;
	case WXK_HOME: kev.keysym = XK_Home; break;
	case WXK_END: kev.keysym = XK_End; break;
	case WXK_PAGEUP: kev.keysym = XK_Page_Up; break;
	case WXK_PAGEDOWN: kev.keysym = XK_Page_Down; break;
	case WXK_F1: kev.keysym = XK_F1; break;
	case WXK_F2: kev.keysym = XK_F2; break;
	case WXK_F3: kev.keysym = XK_F3; break;
	case WXK_F4: kev.keysym = XK_F4; break;
	case WXK_F5: kev.keysym = XK_F5; break;
	case WXK_F6: kev.keysym = XK_F6; break;
	case WXK_F7: kev.keysym = XK_F7; break;
	case WXK_F8: kev.keysym = XK_F8; break;
	case WXK_F9: kev.keysym = XK_F9; break;
	case WXK_F10: kev.keysym = XK_F10; break;
	case WXK_F11: kev.keysym = XK_F11; break;
	case WXK_F12: kev.keysym = XK_F12; break;
	case WXK_F13: kev.keysym = XK_F13; break;
	case WXK_F14: kev.keysym = XK_F14; break;
	case WXK_F15: kev.keysym = XK_F15; break;
	case WXK_NUMLOCK: kev.keysym = XK_Num_Lock; break;
	case WXK_CAPITAL: kev.keysym = XK_Caps_Lock; break;
	case WXK_SCROLL: kev.keysym = XK_Scroll_Lock; break;
	  //case WXK_RSHIFT: kev.keysym = XK_Shift_R; break;
	case WXK_SHIFT: kev.keysym = XK_Shift_L; break;
	  //case WXK_RCTRL: kev.keysym = XK_Control_R; break;
	case WXK_CONTROL: kev.keysym = XK_Control_L; break;
	  //    case WXK_RALT: kev.keysym = XK_Alt_R; break;
	case WXK_ALT: kev.keysym = XK_Alt_L; break;
	  // case WXK_RMETA: kev.keysym = XK_Meta_R; break;
	  // case WXK_META: kev.keysym = XK_Meta_L; break;
	case WXK_WINDOWS_LEFT: kev.keysym = XK_Super_L; break;	
	case WXK_WINDOWS_RIGHT: kev.keysym = XK_Super_R; break;	
	  //case WXK_COMPOSE: kev.keysym = XK_Compose; break;
	  //case WXK_MODE: kev.keysym = XK_Mode_switch; break;
	case WXK_HELP: kev.keysym = XK_Help; break;
	case WXK_PRINT: kev.keysym = XK_Print; break;
	  //case WXK_SYSREQ: kev.keysym = XK_Sys_Req; break;
	case WXK_CANCEL: kev.keysym = XK_Break; break;
	default: break;
	}

      if(kev.keysym)
	{
	  wxLogDebug(wxT("VNCConn %p: got key %s event:"), this, down ? wxT("down") : wxT("up"));
	  wxLogDebug(wxT("VNCConn %p:     wxkeycode:      %d"), this, event.GetKeyCode());
	  wxLogDebug(wxT("VNCConn %p:     wxkeycode char: %c"), this, event.GetKeyCode());
	  wxLogDebug(wxT("VNCConn %p:     unicode:        %d"), this, event.GetUnicodeKey());
	  wxLogDebug(wxT("VNCConn %p:     unicode char:   %c"), this, event.GetUnicodeKey()); 
	  wxLogDebug(wxT("VNCConn %p: sending rfbkeysym:  0x%.3x %s"), this, kev.keysym, down ? wxT("down") : wxT("up"));

	  kev.down = down;
	  if(GetThread() && GetThread()->IsRunning())
	    key_event_q.Post(kev);
	  return true;
	}
      else
	{
	  // nothing of the above?
	  // then propagate this event through the EVT_CHAR handler
	  event.Skip();
	  return false;
	}
    }

  wxLogDebug(wxT("VNCConn %p: no matching keysym found"), this);
  return false; 

}





void VNCConn::doStats(bool yesno)
{
  do_stats = yesno;
  wxCriticalSectionLocker lock(mutex_stats);
  if(do_stats)
    {
      stats_timer.Start(1000);
      latency_test_rect_sent = latency_test_xvpmsg_sent = false; // to start sending one
    }
  else
    stats_timer.Stop();
}


void VNCConn::resetStats()
{
  wxCriticalSectionLocker lock(mutex_stats);
  statistics.Clear();
}




/*
  user input record/replay stuff
 */
 bool VNCConn::replayUserInputStart(wxArrayString src, bool loop)
{
  wxCriticalSectionLocker lock(mutex_recordreplay);

  if(!recording)
    {
      recordreplay_stopwatch.Start();
      userinput_pos = 0;
      userinput = src;
      replay_loop = loop;
      replaying = true;
      return true;
    }
  return false;
}


bool VNCConn::replayUserInputStop()
{
  wxCriticalSectionLocker lock(mutex_recordreplay);

  if(replaying) 
    {
      recordreplay_stopwatch.Pause();
      userinput.Clear();
      replaying = false;
      return true;
    }
  return false;
}


bool VNCConn::recordUserInputStart()
{
   wxCriticalSectionLocker lock(mutex_recordreplay);
  
   if(!replaying)
     {
       recordreplay_stopwatch.Start();
       userinput_pos = 0;
       userinput.Clear();
       recording = true;
       return true;
     }
   return false;
}


bool VNCConn::recordUserInputStop(wxArrayString &dst)
{
   wxCriticalSectionLocker lock(mutex_recordreplay);
 
  if(recording) 
     {
       recordreplay_stopwatch.Pause();
       recording = false;
       dst = userinput; // copy over
       return true;
     }
   return false;
}




/*
  we could use a wxBitmap directly as the framebuffer, thus being more efficient
  BUT:
  - win32 expects DIB data in BGRA, not RGBA (can be solved by requesting another pixel format)
  - win32 DIBs expect data from bottom to top (this means we HAVE to use a self made
    framebuffer -> wxBitmap function instead of GetSubBitmap().)
  - direct data access of a wxBitmap on Mac OS X does not seem to work
  INSTEAD:
  - we have an ordinary char array as framebuffer and copy the requested content into a
    wxBitmap (the return of its copy is cheap cause wxBitmaps use copy-on-write)
 */
wxBitmap VNCConn::getFrameBufferRegion(const wxRect& rect)
{
  wxCriticalSectionLocker lock(mutex_framebuffer);

  // sanity check requested region and client
  if(rect.x < 0 || rect.x + rect.width > getFrameBufferWidth()
     || rect.y < 0 || rect.y + rect.height > getFrameBufferHeight()
     || !cl)
       return wxBitmap();

  /*
    copy directly from framebuffer into a new bitmap
  */

  wxBitmap region(rect.width, rect.height, cl->format.bitsPerPixel);
  wxAlphaPixelData region_data(region);
  wxAlphaPixelData::Iterator region_it(region_data);

  int bytesPerPixel = cl->format.bitsPerPixel/8;
  uint8_t *fbsub_it = cl->frameBuffer + rect.y*cl->width*bytesPerPixel + rect.x*bytesPerPixel;

  for( int y = 0; y < rect.height; ++y )
    {
      wxAlphaPixelData::Iterator region_it_rowStart = region_it;
      uint8_t *fbsub_it_rowStart = fbsub_it;

      for( int x = 0; x < rect.width; ++x, ++region_it, fbsub_it += bytesPerPixel)
	{
	  region_it.Red() = *(fbsub_it+0);	  
	  region_it.Green() = *(fbsub_it+1);	  
	  region_it.Blue() = *(fbsub_it+2);	  
	  region_it.Alpha() = 255; // 255 is opaque, libvncclient always sets this byte to 0
	}
      
      // CR
      region_it = region_it_rowStart;
      fbsub_it = fbsub_it_rowStart;
      
      // LF
      region_it.OffsetY(region_data, 1);
      fbsub_it += cl->width * bytesPerPixel;
    }

  return region;
}




bool VNCConn::getFrameBufferRegion(const wxRect& rect, wxBitmap& dst)
{
  wxCriticalSectionLocker lock(mutex_framebuffer);

  // sanity check requested region against framebuffer
  if(rect.x < 0 || rect.x + rect.width > getFrameBufferWidth()
     || rect.y < 0 || rect.y + rect.height > getFrameBufferHeight())
       return false;

  // check dst against framebuffer
  if(dst.GetWidth() != getFrameBufferWidth() || dst.GetHeight() != getFrameBufferHeight())
    return false;

  /*
    copy directly from framebuffer into the destination bitmap
  */
  wxAlphaPixelData dst_data(dst);
  wxAlphaPixelData::Iterator dst_it(dst_data);
  dst_it.Offset(dst_data, rect.x, rect.y);

  int bytesPerPixel = cl->format.bitsPerPixel/8;
  uint8_t *fbsub_it = cl->frameBuffer + rect.y*cl->width*bytesPerPixel + rect.x*bytesPerPixel;

  for( int y = 0; y < rect.height; ++y )
    {
      wxAlphaPixelData::Iterator dst_it_rowStart = dst_it;
      uint8_t *fbsub_it_rowStart = fbsub_it;

      for( int x = 0; x < rect.width; ++x, ++dst_it, fbsub_it += bytesPerPixel)
	{
	  dst_it.Red() = *(fbsub_it+0);	  
	  dst_it.Green() = *(fbsub_it+1);	  
	  dst_it.Blue() = *(fbsub_it+2);	  
	  dst_it.Alpha() = 255; // 255 is opaque, libvncclient always sets this byte to 0
	}
      
      // CR
      dst_it = dst_it_rowStart;
      fbsub_it = fbsub_it_rowStart;
      
      // LF
      dst_it.OffsetY(dst_data, 1);
      fbsub_it += cl->width * bytesPerPixel;
    }

  return true;
}



int VNCConn::getFrameBufferWidth()
{
  wxCriticalSectionLocker lock(mutex_framebuffer);
  if(cl)
    return cl->width;
  else 
    return 0;
}


int VNCConn::getFrameBufferHeight()
{
  wxCriticalSectionLocker lock(mutex_framebuffer);
  if(cl)
    return cl->height;
  else 
    return 0;
}


int VNCConn::getFrameBufferDepth()
{
  wxCriticalSectionLocker lock(mutex_framebuffer);
  if(cl)
    return cl->format.bitsPerPixel;
  else 
    return 0;
}




wxString VNCConn::getDesktopName() const
{
  if(cl)
    return wxString(cl->desktopName, wxConvUTF8);
  else
    return wxEmptyString;
}


const wxString& VNCConn::getUserName() const {
    return username;
}

void VNCConn::setUserName(const wxString& username) {
    this->username = username;
}

const wxSecretValue& VNCConn::getPassword() const {
    return password;
}

void VNCConn::setPassword(const wxSecretValue& password) {
    this->password = password;
    // tell worker thread to go on
    condition_auth.Signal();
}

const wxString& VNCConn::getSshUserName() const {
    return ssh_username;
}

const wxSecretValue& VNCConn::getSshPassword() const {
    return ssh_password;
}

const wxString& VNCConn::getSshPrivKeyFilename() const {
    return ssh_priv_key_filename;
}

const wxSecretValue& VNCConn::getSshPrivKeyPassword() const {
    return ssh_priv_key_password;
}

const bool VNCConn::getRequireAuth() const {
    return require_auth;
}


wxString VNCConn::getServerHost() const
{
  if(cl)
    {
      if(cl->listenSpecified)
	return wxEmptyString;
      else
	return wxString(cl->serverHost, wxConvUTF8);
    }
  else
    return wxEmptyString;
}



wxString VNCConn::getServerPort() const
{
  if(cl)
    return wxString() << cl->serverPort;
  else
    return wxEmptyString;
}



wxString VNCConn::getListenPort() const
{
  if(cl && cl->listenSpecified)
    return wxString() << cl->listenPort;
  else
    return wxEmptyString;
}

wxString VNCConn::getRepeaterId() const
{
  if(cl && cl->destHost)
    return wxString() << cl->destPort;
  else
    return wxEmptyString;
}

const wxString& VNCConn::getSshHost() const {
    return ssh_host;
}

const wxString& VNCConn::getSshPort() const {
    return ssh_port;
}

bool VNCConn::isMulticast() const
{
  if(cl && cl->multicastSock >= 0 && !cl->multicastDisabled) 
    return true;
  else
    return false;
}


double VNCConn::getMCNACKedRatio()
{
  wxCriticalSectionLocker lock(mutex_multicastratio);
  double retval = 0;
  int samples = 0;
  std::deque<double>::const_iterator it;
  for(it = multicastNACKedRatios.begin(); it != multicastNACKedRatios.end(); ++it)
    if(*it >= 0) // valid value
      {
	retval += *it;
	++samples;
      }

  if(samples)
    return retval/samples;
  else
    return -1;
}


double VNCConn::getMCLossRatio()
{
  wxCriticalSectionLocker lock(mutex_multicastratio);
  double retval = 0;
  int samples = 0;
  std::deque<double>::const_iterator it;
  for(it = multicastLossRatios.begin(); it != multicastLossRatios.end(); ++it)
    if(*it >= 0) // valid value
      {
	retval += *it;
	++samples;
      }
    
  if(samples)
    return retval/samples;
  else
    return -1;
}


void VNCConn::clearLog()
{
  // since we're accessing some global things here from different threads
  wxCriticalSectionLocker lock(mutex_log);
  log.Clear();
}



// get the OS-dependent socket receive buffer size in KByte.
// max returned value is 32MB, negative values on error
int VNCConn::getMaxSocketRecvBufSize()
{
  int sock; 

#ifdef WIN32
  WSADATA trash;
  WSAStartup(MAKEWORD(2,0),&trash);
#endif

  // create the test socket
  if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    return -1;

  // try with some high value and see what we get
  int recv_buf_try = 33554432; // 32 MB
  // This is needed on Linux to see what the maximum value is but errors on MacOS, so we
  // simply treat an error here as non-fatal.
  setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&recv_buf_try, sizeof(recv_buf_try));

  int recv_buf_got = -1;
  socklen_t recv_buf_got_len = sizeof(recv_buf_got);
  if(getsockopt(sock, SOL_SOCKET, SO_RCVBUF,(char*)&recv_buf_got, &recv_buf_got_len) <0)
    {
      rfbCloseSocket(sock);
      return -2;
    } 

  rfbCloseSocket(sock);

  return recv_buf_got/1024;
}


/*
  parse ipv4 or ipv6 address string.
  taken from remmina, thanks!
*/
void VNCConn::parseHostString(const char *server, int defaultport, char **host, int *port)
{
	char *str, *ptr, *ptr2;

	str = strdup(server);

	/* [server]:port format */
	ptr = strchr(str, '[');
	if (ptr)
	{
		ptr++;
		ptr2 = strchr(ptr, ']');
		if (ptr2)
			*ptr2++ = '\0';
		if (*ptr2 == ':')
			defaultport = atoi(ptr2 + 1);
		if (host)
			*host = strdup(ptr);
		if (port)
			*port = defaultport;
		free(str);
		return;
	}

	/* server:port format, IPv6 cannot use this format */
	ptr = strchr(str, ':');
	if (ptr)
	{
		ptr2 = strchr(ptr + 1, ':');
		if (ptr2 == NULL)
		{
			*ptr++ = '\0';
			defaultport = atoi(ptr);
		}
		/* More than one ':' means this is IPv6 address. Treat it as a whole address */
	}
	if (host)
		*host = str;
	if (port)
		*port = defaultport;
}


VNCConn* VNCConn::getVNCConnFromRfbClient(rfbClient *cl) {

    return (VNCConn*) rfbClientGetClientData(cl, VNCCONN_OBJ_ID);

}
