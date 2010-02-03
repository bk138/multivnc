/*
  applicationwide defines for config keys and values 
*/

#ifndef CONFIG_H
#define CONFIG_H


// gui stuff
#define K_SHOWTOOLBAR _T("ShowToolbar")
#define V_SHOWTOOLBAR true
#define K_SHOWDISCOVERED _T("ShowZeroConf")
#define V_SHOWDISCOVERED true
#define K_SHOWBOOKMARKS _T("ShowBookmarks")
#define V_SHOWBOOKMARKS true
#define K_SHOWSTATS _T("ShowStats")
#define V_SHOWSTATS false
#define K_SIZE_X _T("SizeX")
#define V_SIZE_X 800
#define K_SIZE_Y _T("SizeY")
#define V_SIZE_Y 600

// connection settings
#define K_COMPRESSLEVEL wxT("CompressLevel")
#define V_COMPRESSLEVEL 1
#define K_QUALITY wxT("Quality")
#define V_QUALITY 5
#define K_MULTICAST wxT("MulticastVNC")
#define V_MULTICAST true

// logging
#define K_LOGSAVETOFILE wxT("LogFile")
#define V_LOGSAVETOFILE false

// stats settings
#define K_STATSAUTOSAVE wxT("StatsAutosave")
#define V_STATSAUTOSAVE false

//bookmarks
#define G_BOOKMARKS wxT("/Bookmarks/")
#define K_BOOKMARKS_HOST wxT("Host")
#define K_BOOKMARKS_PORT wxT("Port")


#endif // CONFIG_H
