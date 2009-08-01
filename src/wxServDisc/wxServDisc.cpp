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

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <iostream>

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



using namespace std;



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

  if((s = msock()) == 0) 
    { 
      p->err.Printf(_("Can't create socket: %s\n"), strerror(errno));
      exit = true;
    }

  // register query(w,t) at mdnsd d, submit our address for callback ans()
  mdnsd_query(d, w.char_str(), t, ans, this);

  while(!TestDestroy() && !exit)
    {
      tv = mdnsd_sleep(d);
      FD_ZERO(&fds);
      FD_SET(s,&fds);
      select(s+1,&fds,0,0,tv);

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

#ifdef DEBUG
  cerr << "scanthread querying " << w.char_str() << " Entry() end.";
#endif

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
    
#ifdef DEBUG
  cerr << "--->" << endl;
  cerr << "key is: " << key.char_str() << endl; 
  cerr << moi->p->results[key].name.char_str() << endl;
  cerr << inet_ntoa((in_addr&) moi->p->results[key].ip) << endl;
  cerr << moi->p->results[key].port << endl;
  cerr << "<---" << endl;
#endif

  return 1;
}



// create a multicast 224.0.0.251:5353 socket, windows or unix style
SOCKET ScanThread::msock() const
{
	SOCKET s;
	int flag = 1;
	char ttl = 255; // Used to set multicast TTL
	int ittl = 255;

	// this is our local address
	struct sockaddr_in addrLocal;
        memset(&addrLocal, '\0', sizeof(addrLocal));
	addrLocal.sin_family = AF_INET;
	addrLocal.sin_port = htons(5353);
        addrLocal.sin_addr.s_addr = 0;	

	// and this the multicast destination
	struct ip_mreq	ipmr;
	ipmr.imr_multiaddr.s_addr = inet_addr("224.0.0.251");
	ipmr.imr_interface.s_addr = htonl(INADDR_ANY);

#ifdef __WIN32__
	// winsock startup
	WORD			wVersionRequested;
	WSADATA			wsaData;
	wVersionRequested = MAKEWORD(2, 2);
	if(WSAStartup(wVersionRequested, &wsaData) != 0)
	{
		WSACleanup();
		printf("Failed to start winsock\r\n");
		return 0;
	}
#endif


	// Create a new socket
#ifdef __WIN32__
	if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
#else
        if((s = socket(AF_INET,SOCK_DGRAM, IPPROTO_UDP)) < 0)
#endif
	return 0;
	

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
                return 0;
        }

  	// Set the multicast ttl
	setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
        setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ittl, sizeof(ittl));

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
  static_cast<ScanThread*>(scanthread)->Delete();
#ifdef DEBUG
  cerr << "wxservdisc destroyed, query was " << query.char_str() << endl; 
#endif
}





vector<wxSDEntry> wxServDisc::getResults() const
{
  vector<wxSDEntry> resvec;

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
  // new NOTIFY event, we got no window id
  wxCommandEvent event(wxServDiscNOTIFY, wxID_ANY);
  event.SetEventObject(this); // set sender

  // Send it
  wxPostEvent((wxEvtHandler*)parent, event);
}




