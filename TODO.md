
# wxWidgets version:
 - make fullscreen behave really like fullscreen
 - wxTaskBarIcon
 - fix hang when connection is disrupted
 - make VNC server on win32 exit when window is closed
 - fix the seamless connector for win32.
 - better bookmarks using wxListCtrl
 - make it work on OS X
 - textchat
 - server side scaling
 - multiple connections at once, like the other multivnc
 - more tooltips
 - translations
 - new drawing model:

                          wxBitmap(per viewerwindow) -> (wxImage-> scaled wxImage -> wxBitmap) -> screen
                        /
        cl->framebuffer
                        \
                          gl texture(per viewerwindow) -> screen (via wxGLCanvas -> FAST scaling)
                  
 read directly cl->framebuffer into wxBitmap/texture WITHOUT an intermediate wxBitmap object

 per viewerwindow wxBitmap/texture to screen happens at fixed interval so no more overload of
 drawing engine

 - stutter when using stats on win32 -> seems to be caused by
   SendXvpMessage(), but why? testrect case is ok...
   
   
# Android version:

* port to libvncclient
- unified mode of operation (?)

  
