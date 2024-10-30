/* 
   MultiVNCApp.cpp: MultiVNC main app implementation.

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


#include <iostream>
#include <csignal>
#include <wx/wfstream.h>
#ifdef __WXMAC__
#include <ApplicationServices/ApplicationServices.h>
#endif
#include <wx/stdpaths.h>
#include "MultiVNCApp.h"
#include "DebugReportEmail.h"
#include "gui/MyFrameMain.h"


using namespace std;



static void handle_sig(int s)
{
  switch(s)
    {
    case SIGINT:
      wxGetApp().nr_sigints++;
      cerr << "Got SIGINT, shutting down\n";
      wxGetApp().ExitMainLoop();
      if(wxGetApp().nr_sigints >= 3)
	{
	  cerr << "Got 3 SIGINTs, killing myse...\n";
#ifndef __WXMSW__
	  raise(SIGKILL);
#else
	  raise(SIGTERM);
#endif
	}
      break;
    }
}



// this also sets up main()
IMPLEMENT_APP(MultiVNCApp);



bool MultiVNCApp::OnInit()
{
  locale = 0;

  setLocale(wxLANGUAGE_DEFAULT);

  // setup signal handlers
  nr_sigints = 0;
  signal(SIGINT, handle_sig);
#if !defined __WXMSW__ || defined __BORLANDC__ || defined _MSC_VER // on win32, does only work with borland c++ or visual c++
  wxHandleFatalExceptions(true); // enable ::OnFatalException() which handles fatal signals
#endif

  // wxConfig:
  // application and vendor name are used by wxConfig to construct the name
  // of the config file/registry key and must be set before the first call
  // to Get() if you want to override the default values (the application
  // name is the name of the executable and the vendor name is the same)
  SetVendorName(_T("MultiVNC"));

  // https://forums.wxwidgets.org/viewtopic.php?t=42605
  SetAppDisplayName("MultiVNC");

  // if built as a portable edition, use file config always!
#ifdef PORTABLE_EDITION
  const char* name = CFGFILE;
  wxFile cfgfile;

  if(!wxFile::Exists(wxString(name, wxConvUTF8)))
    cfgfile.Create(wxString(name, wxConvUTF8));

  cfgfile.Open(wxT(CFGFILE), wxFile::read);

  wxFileInputStream sin(cfgfile);
  if(sin.Ok())
    {
      wxFileConfig* cfg = new wxFileConfig(sin);
      wxConfigBase::Set(cfg);
    }
  else
    {
      wxLogError(_("Could not open config file!"));
    }
#endif

  // use XDG layout if running as flatpak or there is no old-style config file already
  if(wxGetEnv("FLATPAK_ID", NULL) || !wxFile::Exists(wxFileName::GetHomeDir() + wxFileName::GetPathSeparator() + ".multivnc"))
      wxStandardPaths::Get().SetFileLayout(wxStandardPaths::FileLayout_XDG);

  // greetings to anyone who made it...
  cout << "\n:::  this is MultiVNC  :::\n\n";
  cout << COPYRIGHT << ".\n";
  cout << "MultiVNC is free software, licensed unter the GPL.\n\n";


  // wx stuff
  wxInitAllImageHandlers();

  // the main frame
  MyFrameMain* frame_main = new MyFrameMain(NULL, wxID_ANY, wxEmptyString);

  for(int i=1; i < wxApp::argc; ++i)
    {
      wxString arg(wxApp::argv[i]);
      frame_main->cmdline_connect(arg);
    }

  SetTopWindow(frame_main);
  frame_main->Show();

  return true;
}


int MultiVNCApp::OnExit()
{
  // if built as a portable edition, use file config always!
#ifdef PORTABLE_EDITION
  wxFile cfgfile(wxT(CFGFILE), wxFile::write);
  wxFileOutputStream sout(cfgfile);
  wxFileConfig* cfg = (wxFileConfig*)wxConfigBase::Get();
  if(!cfg->Save(sout))
    wxLogError(_("Could not save config file!"));
#endif

  // clean up: Set() returns the active config object as Get() does, but unlike
  // Get() it doesn't try to create one if there is none (definitely not what
  // we want here!)
  delete wxConfigBase::Set((wxConfigBase *) NULL);

  return 0;
}



void MultiVNCApp::OnUnhandledException()
{
  //this way it should work under both normal and debug builds
  wxString msg = _("GAAH! Got an unhandled exception! This should not happen."); 
  wxLogError(msg);
  wxLogDebug(msg);
}


void MultiVNCApp::OnFatalException()
{
  int answer = wxMessageBox(_("Ouch! MultiVNC crashed. This should not happen. Do you want to generate a bug report?"), _("MultiVNC crashed"),
			    wxICON_ERROR | wxYES_NO, NULL);
  if(answer == wxYES)
    genDebugReport(wxDebugReport::Context_Exception);
}


void MultiVNCApp::genDebugReport(wxDebugReport::Context ctx)
{
  DebugReportEmail report("multivnc@christianbeier.net", "MultiVNC Bug Report");

  // add all standard files: currently this means just a minidump and an
  // XML file with system info and stack trace
  report.AddAll(ctx);

  // calling Show() is not mandatory, but is more polite
  if(wxDebugReportPreviewStd().Show(report))
      report.Process();
}




bool MultiVNCApp::setLocale(int language)
{
  delete locale;
     
  locale = new wxLocale;

  // don't use wxLOCALE_LOAD_DEFAULT flag so that Init() doesn't return 
  // false just because it failed to load wxstd catalog                                 
  if(! locale->Init(language) )                      
    return false;    

  // normally this wouldn't be necessary as the catalog files would be found  
  // in the default locations, but when the program is not installed the
  // catalogs are in the build directory where we wouldn't find them by  
  // default
  wxLocale::AddCatalogLookupPathPrefix(wxT("."));            
           
  // Initialize the catalogs we'll be using  
  locale->AddCatalog(wxT("multivnc"));  

  return true;
}


