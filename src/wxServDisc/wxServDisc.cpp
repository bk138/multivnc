/* 
   wxServDisc.cpp: wxServDisc implementation

   This file is part of wxServDisc, a crossplatform wxWidgets
   Zeroconf service discovery module.
 
   Copyright (C) 2008 Christian Beier <dontmind@freeshell.org>
 
   wxServDisc is free software; you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation; either version 2 of the License, or 
   (at your option) any later version. 
 
   wxServDisc is distributed in the hope that it will be useful, 
   but WITHOUT ANY WARRANTY; without even the implied warranty of 
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
   GNU General Public License for more details. 
 
   You should have received a copy of the GNU General Public License 
   along with this program; if not, write to the Free Software 
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. 
*/


#include "wx/object.h"
#include "wx/thread.h"
#include "wx/intl.h"
#include "wx/log.h"

#include <fcntl.h>
#include <cerrno>
#include <csignal>


#ifdef __WIN32__
// mingw socket includes
#include <winsock2.h>
#include <ws2tcpip.h>
#else
// unix socket includes
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif


#include "wxServDisc.h"
#include "mdnsd.h"

// just for convenience
// SOCKET is a unsigned int in win32!!
// but in unix we expect signed ints!!
#ifndef __WIN32__
typedef int SOCKET;
#endif


// define our new notify event!
DEFINE_EVENT_TYPE(wxServDiscNOTIFY)






/*
  internal scanner thread class
*/
class ScanThread : public wxThread
{
  wxServDisc *p; // our parent object
  wxString w;   // query what?
  int t;        // query type

  // create a multicast 224.0.0.251:5353 socket, windows or unix style
  SOCKET msock() const; 
  // send/receive message m
  bool sendm(struct message *m, SOCKET s, unsigned long int ip, unsigned short int port);
  int recvm(struct message *m, SOCKET s, unsigned long int *ip, unsigned short int *port);
  
  static int ans(mdnsda a, void *caller);


public:
  ScanThread(const wxString& what, int type, wxServDisc *parent);
  
  // thread execution starts here
  virtual void* Entry();
  
  // called when the thread exits - whether it terminates normally or is
  // stopped with Delete() (but not when it is Kill()ed!)
  virtual void OnExit();
};



ScanThread::ScanThread(const wxString& what, int type, wxServDisc *parent)
  : wxThread()
{
  p = parent;
  w = what;
  t = type;
}



void* ScanThread::Entry()
{
  mdnsd d;
  struct message m;
  unsigned long int ip;
  unsigned short int port;
  struct timeval *tv;
  fd_set fds;
  SOCKET s;
  bool exit = false;


  d = mdnsd_new(1,1000);

  if((s = msock()) < 0) 
    { 
      p->err.Printf(_("Can't create socket: %s\n"), strerror(errno));
      exit = true;
    }

  // register query(w,t) at mdnsd d, submit our address for callback ans()
  mdnsd_query(d, w.char_str(), t, ans, this);


#ifdef __WXGTK__
  // this signal is generated when we pop up a file dialog wwith wxGTK
  // we need to block it here cause it interrupts the select() call
  sigset_t            newsigs;
  sigset_t            oldsigs;
  sigemptyset(&newsigs);
  sigemptyset(&oldsigs);
  sigaddset(&newsigs, SIGRTMIN-1);
#endif


  while(!TestDestroy() && !exit)
    {
      tv = mdnsd_sleep(d);
    
      long msecs = tv->tv_sec == 0 ? 100 : tv->tv_sec*1000; // so that the while loop beneath gets executed once
      wxLogDebug(wxT("wxServDisc %p: scanthread waiting for data, timeout %i seconds"), p, tv->tv_sec);


      // we split the one select() call into several ones every 100ms
      // to be able to catch TestDestroy()...
      int datatoread = 0;
      while(msecs > 0 && !TestDestroy() && !datatoread)
	{
	  // the select call leaves tv undefined, so re-set
	  tv->tv_sec = 0;
	  tv->tv_usec = 100000; // 100 ms

	  FD_ZERO(&fds);
	  FD_SET(s,&fds);


#ifdef __WXGTK__
	  sigprocmask(SIG_BLOCK, &newsigs, &oldsigs);
#endif
	  datatoread = select(s+1,&fds,0,0,tv); // returns 0 if timeout expired

#ifdef __WXGTK__
	  sigprocmask(SIG_SETMASK, &oldsigs, NULL);
#endif

	  if(!datatoread) // this is a timeout
	    msecs-=100;
	  if(datatoread == -1)
	    break;
	}
      
      wxLogDebug(wxT("wxServDisc %p: scanthread woke up, reason: incoming data(%i), timeout(%i), error(%i), deletion(%i)"),
		 p, datatoread>0, msecs<=0, datatoread==-1, TestDestroy() );

      // receive
      if(FD_ISSET(s,&fds))
        {
	  while(recvm(&m, s, &ip, &port) > 0)
	    mdnsd_in(d, &m, ip, port);
        }

      // send
      while(mdnsd_out(d,&m,&ip,&port))
	if(!sendm(&m, s, ip, port))
	  {
	    exit = true;
	    break;
	  }
    }

  mdnsd_shutdown(d);
  mdnsd_free(d);

  return NULL;
}




bool ScanThread::sendm(struct message* m, SOCKET s, unsigned long int ip, unsigned short int port)
{
  struct sockaddr_in to;
 
  memset(&to, '\0', sizeof(to));

  to.sin_family = AF_INET;
  to.sin_port = port;
  to.sin_addr.s_addr = ip;

  if(sendto(s, (char*)message_packet(m), message_packet_len(m), 0,(struct sockaddr *)&to,sizeof(struct sockaddr_in)) != message_packet_len(m))  
    { 
      p->err.Printf(_("Can't write to socket: %s\n"),strerror(errno));
      return false;
    }

  return true;
}





int ScanThread::recvm(struct message* m, SOCKET s, unsigned long int *ip, unsigned short int *port) 
{
  struct sockaddr_in from;
  int bsize;
  static unsigned char buf[MAX_PACKET_LEN];
#ifdef __WIN32__
  int ssize  = sizeof(struct sockaddr_in);
#else
  socklen_t ssize  = sizeof(struct sockaddr_in);
#endif
 

  if((bsize = recvfrom(s, (char*)buf, MAX_PACKET_LEN, 0, (struct sockaddr*)&from, &ssize)) > 0)
    {
      memset(m, '\0', sizeof(struct message));
      message_parse(m,buf);
      *ip = (unsigned long int)from.sin_addr.s_addr;
      *port = from.sin_port;
      return bsize;
    }

#ifdef __WIN32__
  if(bsize < 0 && WSAGetLastError() != WSAEWOULDBLOCK) 
#else
    if(bsize < 0 && errno != EAGAIN)
#endif
      {
	p->err.Printf(_("Can't read from socket %d: %s\n"),
		      errno,strerror(errno));
	return bsize;
      }

  return 0;
}







int ScanThread::ans(mdnsda a, void *arg)
{
  ScanThread *moi = (ScanThread*)arg;
  

  wxString key;
  switch(a->type)
    {
    case QTYPE_PTR:
      // query result is key
      key = wxString((char*)a->rdname, wxConvUTF8); 
      break;
    case QTYPE_A:
    case QTYPE_SRV:
      // query name is key
      key = wxString((char*)a->name, wxConvUTF8);
      break;
    default:
      break;
    }


  // insert answer data into result
  // depending on the query, not all fields have a meaning
  wxSDEntry result;

  result.name = wxString((char*)a->rdname, wxConvUTF8);

  struct in_addr ip;
  ip.s_addr =  ntohl(a->ip);
  result.ip = wxString(inet_ntoa(ip), wxConvUTF8); 
 
  result.port = a->srv.port;


  if(a->ttl == 0)
    // entry was expired
    moi->p->results.erase(key);
  else
    // entry update
    moi->p->results[key] = result;

  moi->p->SendNotify();
    
  
  wxLogDebug(wxT("wxServDisc %p: got answer:"), moi->p);
  wxLogDebug(wxT("wxServDisc %p:    key:  %s"), moi->p, key.c_str());
  wxLogDebug(wxT("wxServDisc %p:    name: %s"), moi->p, moi->p->results[key].name.c_str());
  wxLogDebug(wxT("wxServDisc %p:    ip:   %s"), moi->p, moi->p->results[key].ip.c_str());
  wxLogDebug(wxT("wxServDisc %p:    port: %u"), moi->p, moi->p->results[key].port);
  wxLogDebug(wxT("wxServDisc %p: answer end"), moi->p);
  
  return 1;
}



// create a multicast 224.0.0.251:5353 socket,
// aproppriate for receiving and sending,
// windows or unix style
SOCKET ScanThread::msock() const
{
  SOCKET s;
  int flag = 1;
  int ttl = 255; // multicast TTL, must be 255 for zeroconf!

  // this is our local address
  struct sockaddr_in addrLocal;
  memset(&addrLocal, 0, sizeof(addrLocal));
  addrLocal.sin_family = AF_INET;
  addrLocal.sin_port = htons(5353);
  addrLocal.sin_addr.s_addr = htonl(INADDR_ANY);	

  // and this the multicast destination
  struct ip_mreq	ipmr;
  ipmr.imr_multiaddr.s_addr = inet_addr("224.0.0.251");
  ipmr.imr_interface.s_addr = htonl(INADDR_ANY);

#ifdef __WIN32__
  // winsock startup
  WORD		wVersionRequested;
  WSADATA	wsaData;
  wVersionRequested = MAKEWORD(2, 2);
  if(WSAStartup(wVersionRequested, &wsaData) != 0)
    {
      WSACleanup();
      wxLogError(wxT("Failed to start winsock"));
      return -1;
    }
#endif


  // Create a new socket
  if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    return -1;

  // set to reuse address
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag));

  // Bind socket to port, returns 0 on success
  if(bind(s, (struct sockaddr*) &addrLocal, sizeof(addrLocal))) 
    { 
#ifdef __WIN32__
      closesocket(s);
#else
      close(s);
#endif 
      return -1;
    }

  // Set the multicast ttl
  setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ttl, sizeof(ttl));

  // Add socket to be a member of the multicast group
  setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&ipmr, sizeof(ipmr));
	
  // set to nonblock
#ifdef __WIN32__
  unsigned long block=1;
  ioctlsocket(s, FIONBIO, &block);
#else
  flag =  fcntl(s, F_GETFL, 0);
  flag |= O_NONBLOCK;
  fcntl(s, F_SETFL, flag);
#endif
	
  // whooaa, that's it
  return s;
}






void ScanThread::OnExit()
{
  wxLogDebug(wxT("wxServDisc %p: scanthread querying '%s' exiting"), p, w.c_str());
  p->scanthread = 0;
}






/*
  wxServDisc class

*/


wxServDisc::wxServDisc(void* p, const wxString& what, int type)
{
  // save our caller
  parent = p;

  // save query
  query = what;

  wxLogDebug(wxT(""));
  wxLogDebug(wxT("wxServDisc %p: about to query '%s'"), this, query.c_str());

  ScanThread *st_ptr = new ScanThread(what, type, this);

  // save it for later on
  scanthread = st_ptr;
  
  if( st_ptr->Create() != wxTHREAD_NO_ERROR )
    err.Printf(_("Can't create scan thread!"));

  if( st_ptr->Run() != wxTHREAD_NO_ERROR )
    err.Printf(_("Can't start scan thread!")); 
}



wxServDisc::~wxServDisc()
{
  wxLogDebug(wxT("wxServDisc %p: before scanthread delete"), this);
  static_cast<ScanThread*>(scanthread)->Delete();
  // wait for deletion to finish
  while(scanthread)
    wxMilliSleep(100);

  wxLogDebug(wxT("wxServDisc %p: scanthread deleted, wxServDisc destroyed, query was '%s'"), this, query.c_str());
  wxLogDebug(wxT("")); 
}





std::vector<wxSDEntry> wxServDisc::getResults() const
{
  std::vector<wxSDEntry> resvec;

  wxSDMap::const_iterator it;
  for(it = results.begin(); it != results.end(); it++)
    resvec.push_back(it->second);

  return resvec;
}



size_t wxServDisc::getResultCount() const
{
  return results.size();
}





void wxServDisc::SendNotify()
{
  if(parent)
    {
      // new NOTIFY event, we got no window id
      wxCommandEvent event(wxServDiscNOTIFY, wxID_ANY);
      event.SetEventObject(this); // set sender
      
      // Send it
      wxPostEvent((wxEvtHandler*)parent, event);
    }
}




