// -*- C++ -*- 
/* 
   MultiVNCApp.h: MultiVNC main app header.

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

#ifndef MULTIVNCAPP_H
#define MULTIVNCAPP_H


#include <wx/wxprec.h>
#ifndef WX_PRECOMP 
#include "wx/wx.h"       
#endif
#include "wx/debugrpt.h"
#include "wx/config.h"
#include "config.h"

// in case we have an old autoconf...
#ifndef PACKAGE_URL
#define PACKAGE_URL "http://multivnc.sf.net"
#endif

#define COPYRIGHT "Copyright (C) 2009-2024 Christian Beier <multivnc@christianbeier.net>"
#define CFGFILE "multivnc.cfg"

class MultiVNCApp: public wxApp 
{
  wxLocale *locale;
  
public:
  
  virtual bool OnInit();
  virtual int  OnExit();
  virtual void OnUnhandledException();
  virtual void OnFatalException();
  size_t nr_sigints;
  
  // this is where we really generate the debug report
  void genDebugReport(wxDebugReport::Context ctx);
  
  bool setLocale(int language);

};


DECLARE_APP(MultiVNCApp);





#endif // MULTIVNCAPP_H
