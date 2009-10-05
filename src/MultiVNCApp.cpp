/*


*/

#include <iostream>


#include "MultiVNCApp.h"
#include "gui/MyFrameMain.h"


using namespace std;



// this also sets up main()
IMPLEMENT_APP(MultiVNCApp);



bool MultiVNCApp::OnInit()
{
  locale = 0;
  
  setLocale(wxLANGUAGE_DEFAULT);

  // wxConfig:
  // application and vendor name are used by wxConfig to construct the name
  // of the config file/registry key and must be set before the first call
  // to Get() if you want to override the default values (the application
  // name is the name of the executable and the vendor name is the same)
  SetVendorName(_T("MultiVNC"));
 
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
      if(! frame_main->cmdline_connect(arg))
	return false;
    }

  SetTopWindow(frame_main);
  frame_main->Show();

  return true;
}


int MultiVNCApp::OnExit()
{
  // clean up: Set() returns the active config object as Get() does, but unlike
  // Get() it doesn't try to create one if there is none (definitely not what
  // we want here!)
  delete wxConfigBase::Set((wxConfigBase *) NULL);

  return 0;
}



bool MultiVNCApp::setLocale(int language)
{
  delete locale;
     
  locale = new wxLocale;

  // don't use wxLOCALE_LOAD_DEFAULT flag so that Init() doesn't return 
  // false just because it failed to load wxstd catalog                                 
  if(! locale->Init(language, wxLOCALE_CONV_ENCODING) )                      
    {  
      wxLogError(_("This language is not supported by the system.")); 
      return false;    
    }            

  // normally this wouldn't be necessary as the catalog files would be found  
  // in the default locations, but when the program is not installed the
  // catalogs are in the build directory where we wouldn't find them by  
  // default
  wxLocale::AddCatalogLookupPathPrefix(wxT("."));            
           
  // Initialize the catalogs we'll be using  
  locale->AddCatalog(wxT("multivnc"));  

  return true;
}
