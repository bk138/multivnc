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
#define K_SHOW1TO1 "Show1To1"
#define V_SHOW1TO1 false
#define K_SIZE_X _T("SizeX")
#define V_SIZE_X 1024
#define K_SIZE_Y _T("SizeY")
#define V_SIZE_Y 768

// connection settings
#define K_MULTICAST wxT("MulticastVNC")
#define V_MULTICAST true
#define K_MULTICASTSOCKETRECVBUF wxT("MulticastSocketRecvBufSize")
#define V_MULTICASTSOCKETRECVBUF 5120
#define K_MULTICASTRECVBUF wxT("MulticastRecvBufSize")
#define V_MULTICASTRECVBUF 5120
#define K_FASTREQUEST wxT("FastRequest")
#define V_FASTREQUEST true
#define K_FASTREQUESTINTERVAL wxT("FastRequestInterval")
#define V_FASTREQUESTINTERVAL 30
#define K_QOS_EF wxT("QOS_EF")
#define V_QOS_EF true
#define K_GRABKEYBOARD wxT("GrabKeyboard")
#define V_GRABKEYBOARD false
#define K_LASTHOST "LastHost"

// encodings settings
#define K_ENC_COPYRECT wxT("EncodingCopyRect")
#define V_ENC_COPYRECT true
#define K_ENC_RRE wxT("EncodingRRE")
#define V_ENC_RRE true
#define K_ENC_HEXTILE wxT("EncodingHextile")
#define V_ENC_HEXTILE true
#define K_ENC_CORRE wxT("EncodingCorre")
#define V_ENC_CORRE true
#define K_ENC_ZLIB wxT("EncodingZlib")
#define V_ENC_ZLIB true
#define K_ENC_ZLIBHEX wxT("EncodingZlibhex")
#define V_ENC_ZLIBHEX true
#define K_ENC_ZRLE wxT("EncodingZRLE")
#define V_ENC_ZRLE true
#define K_ENC_ZYWRLE wxT("EncodingZYWRLE")
#define V_ENC_ZYWRLE true
#define K_ENC_ULTRA wxT("EncodingUltra")
#define V_ENC_ULTRA true
#define K_ENC_TIGHT wxT("EncodingTight")
#define V_ENC_TIGHT true
#define K_COMPRESSLEVEL wxT("CompressLevel")
#define V_COMPRESSLEVEL 1
#define K_QUALITY wxT("Quality")
#define V_QUALITY 5

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
#define K_BOOKMARKS_USER "User"

// collab features
#define K_WINDOWSHARE _T("WindowShareCmd")
#ifdef __WIN32__
#define V_DFLTWINDOWSHARE _T("windowshare.exe -oneshot -sharewindow \"%w\" -connect %a")
#else
#define V_DFLTWINDOWSHARE _T("x11vnc -repeat -viewonly -id pick -xrandr -connect_or_exit %a")
#endif


#endif // DFLTCFG_H
