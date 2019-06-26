[![Build Status](https://travis-ci.org/bk138/multivnc.svg?branch=master)](https://travis-ci.org/bk138/multivnc)
[![Help making this possible](https://img.shields.io/badge/liberapay-donate-yellow.png)](https://liberapay.com/bk138/donate)
[![Become a patron](https://img.shields.io/badge/patreon-donate-yellow.svg)](https://www.patreon.com/bk138)
[![Donate](https://img.shields.io/badge/paypal-donate-yellow.png)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=N7GSSPRPUSTPU&source=url)

MultiVNC is a cross-platform Multicast-enabled VNC viewer using
wxWidgets and libvncclient . It runs on Unix, Mac OS X and Windows.
There also is an [Android version](/android/).

Both the desktop and the mobile version feature:

* Support for most encodings including Tight.
* Discovery of VNC servers advertising themselves via ZeroConf. 
* Bookmarks.

The desktop version stands out with additional features such as:

* MulticastVNC ;-) 
* Several connections with one viewer using tabs.
* Listen mode (Reverse VNC). Via tabs it's possible to listen 
  for and serve multiple incoming connections.
* Window sharing: You can beam one of your windows to the remote
  side if they support receiving windows (run a listening viewer).
* Seamless control of the remote side by moving pointer over the
  (default upper) screen edge. Borrows heavily from x2vnc by
  Fredrik Hübinette <hubbe@hubbe.net>, which in turn was based on
  ideas from x2x and code from vncviewer.
* Simple, loggable statistics 
* Supports server framebuffer resize.

The mobile Android version sports:

* Virtual mouse button controls with haptic feedback.
* Two-finger swipe gesture recognition.
* A super fast touchpad mode for local use.
* Hardware-accelerated OpenGL drawing and zooming.
* Copy&paste to and from Android.
* Availability from both [Google Play](https://play.google.com/store/apps/details?id=com.coboltforge.dontmind.multivnc)
  and [F-Droid](https://f-droid.org/packages/com.coboltforge.dontmind.multivnc/).

For features that are planned, but not completed yet, look at the
[TODO](TODO.md) file.




# MulticastVNC notes

You can get a modified libvncserver/libvncclient at
https://github.com/LibVNC/libvncserver/tree/multicastvnc -
this is the same library that MultiVNC uses internally.




# How to compile the desktop wxWidgets version from source

The prerequisites:

* the usual c-compiler with headers and stuff
* wxWidgets dev package version >= 2.8.7
* zlib dev package 
* libjpeg dev package 

After cloning the repo, do

```
   git submodule init
   git submodule update
```

To compile:

```
   mkdir build
   cd build
   cmake ..
   cmake --build .
```

And cross fingers...


To install:
* 'make install' as root
* or copy binary wherever you like to



That's pretty much it, have fun !
