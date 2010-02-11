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
#include "wx/config.h"



#define VERSION "0.2"
#define COPYRIGHT "Copyright (C) 2009, 2010 Christian Beier <dontmind@freeshell.org>"


class MultiVNCApp: public wxApp 
{
  wxLocale *locale;
public:
  
  bool OnInit();
  int  OnExit();
  virtual void OnUnhandledException();
 
  bool setLocale(int language);

  // application-wide mutex protecting wxTheClipboard
  wxCriticalSection mutex_theclipboard;
};


DECLARE_APP(MultiVNCApp);





#endif // MULTIVNCAPP_H
