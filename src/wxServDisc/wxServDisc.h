// -*- C++ -*- 
/* 
   wxServDisc.h: wxServDisc API definition

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

#ifndef WXSERVDISC_H
#define WXSERVDISC_H

#include "wx/event.h"
#include "wx/string.h"
#include "wx/hashmap.h"
#include <vector>

#include "1035.h"

// make available custom notify event if getResults() would yield sth new
DECLARE_EVENT_TYPE(wxServDiscNOTIFY, -1)



// resource name with ip addr and port number
struct wxSDEntry
{
  wxString name;
  wxString ip;
  uint16_t port;
  wxSDEntry() { port=0; }
};


// a string hash map containing these entries
WX_DECLARE_STRING_HASH_MAP(wxSDEntry, wxSDMap);


// our main class
class wxServDisc: public wxObject
{
  friend class ScanThread;
  void* scanthread;
  wxString err;
  void *parent;
  wxString query;
  wxSDMap results;

  void SendNotify();

public:
  // type can be one of QTYPE_A, QTYPE_NS, QTYPE_CNAME, QTYPE_PTR or QTYPE_SRV 
  wxServDisc(void* parent, const wxString& what, int type);
  ~wxServDisc();
  

  // yeah well...
  std::vector<wxSDEntry> getResults() const;
  size_t getResultCount() const;


  // get query name
  const wxString& getQuery() const { const wxString& ref = query; return ref; };
  // get error string
  const wxString& getErr() const { const wxString& ref = err; return ref; };
};



#endif // WXSERVDISC_H












