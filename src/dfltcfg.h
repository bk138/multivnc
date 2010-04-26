/* 
   dftlcfg.h: applicationwide defines for config keys and values.  

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


#ifndef DFLTCFG_H
#define DFLTCFG_H


// gui stuff
#define K_SHOWTOOLBAR _T("ShowToolbar")
#define V_SHOWTOOLBAR true
#define K_SHOWDISCOVERED _T("ShowZeroConf")
#define V_SHOWDISCOVERED true
#define K_SHOWBOOKMARKS _T("ShowBookmarks")
#define V_SHOWBOOKMARKS true
#define K_SHOWSTATS _T("ShowStats")
#define V_SHOWSTATS false
#define K_SHOWSEAMLESS _T("ShowSeamless")
#define V_SHOWSEAMLESS EDGE_NONE
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
#define K_MULTICASTRECVBUF wxT("MulticastRecvBufSize")
#define V_MULTICASTRECVBUF 325
#define K_FASTREQUEST wxT("FastRequest")
#define V_FASTREQUEST false
#define K_FASTREQUESTINTERVAL wxT("FastRequestInterval")
#define V_FASTREQUESTINTERVAL 30

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

// collab features
#define K_WINDOWSHARE _T("WindowShareCmd")
#ifdef __WIN32__
#define V_DFLTWINDOWSHARE _T("winvnc.exe -oneshot -sharewindow \"%w\" -connect %a")
#else
#define V_DFLTWINDOWSHARE _T("x11vnc -repeat -sid pick -xrandr -connect_or_exit %a")
#endif


#endif // DFLTCFG_H
