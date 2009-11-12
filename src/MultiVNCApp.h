#ifndef MULTIVNCAPP_H
#define MULTIVNCAPP_H


#include <wx/wxprec.h>
#ifndef WX_PRECOMP 
#include "wx/wx.h"       
#endif  
#include "wx/config.h"



#define VERSION "0.2"
#define COPYRIGHT "Copyright (C) 2009 Christian Beier <dontmind@freeshell.org>"


class MultiVNCApp: public wxApp 
{
  wxLocale *locale;
public:
  
  bool OnInit();
  int  OnExit();
  bool setLocale(int language);

  // application-wide mutex protecting wxTheClipboard
  wxCriticalSection mutex_theclipboard;
};


DECLARE_APP(MultiVNCApp);





#endif // MULTIVNCAPP_H
