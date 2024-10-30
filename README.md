
# MultiVNC

[![CI](https://github.com/bk138/multivnc/actions/workflows/ci.yml/badge.svg)](https://github.com/bk138/multivnc/actions/workflows/ci.yml)
[![Help making this possible](https://img.shields.io/badge/liberapay-donate-yellow.png)](https://liberapay.com/bk138/donate)
[![Become a patron](https://img.shields.io/badge/patreon-donate-yellow.svg)](https://www.patreon.com/bk138)
[![Donate](https://img.shields.io/badge/paypal-donate-yellow.png)](https://www.paypal.com/donate/?hosted_button_id=HKRTWKNKBKPKN)
[![Gitter](https://badges.gitter.im/multivnc/community.svg)](https://gitter.im/multivnc/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)

MultiVNC is a cross-platform Multicast-enabled VNC viewer based on
[LibVNCClient](https://github.com/LibVNC/libvncserver). The desktop version
uses [wxWidgets](https://www.wxwidgets.org/) and runs on Unix, Mac OS X and
Windows. There also is an [Android version](/android/).

The roadmap for future developments regarding the project can be found
[here](https://github.com/bk138/multivnc/projects).

## MultiVNC for Android

### Features

* Support for most VNC encodings including Tight.
* TLS support, i.e. AnonTLS and VeNCrypt.
* Support for SSH-Tunnelling with password- and privkey-based authentication.
* UltraVNC Repeater support.
* Discovery of VNC servers advertising themselves via ZeroConf.
* Bookmarking of connections.
* Import and export of saved connections.
* Virtual mouse button controls with haptic feedback.
* Two-finger swipe gesture recognition.
* A super fast touchpad mode for local use.
* Hardware-accelerated OpenGL drawing and zooming.
* Supports server framebuffer resize.
* Copy&paste to and from Android.

### How to get it

[<img src="https://fdroid.gitlab.io/artwork/badge/get-it-on.png"
     alt="Get it on F-Droid"
     height="80">](https://f-droid.org/packages/com.coboltforge.dontmind.multivnc/)
[<img src="https://play.google.com/intl/en_us/badges/images/generic/en-play-badge.png"
     alt="Get it on Google Play"
     height="80">](https://play.google.com/store/apps/details?id=com.coboltforge.dontmind.multivnc)

### How to build

See the [Android version's README](android/README.md).

## MultiVNC for Desktop

### Features

* Support for most encodings including Tight.
* TLS support, i.e. AnonTLS and VeNCrypt.
* Discovery of VNC servers advertising themselves via ZeroConf. 
* Bookmarking of connections.
* Supports server framebuffer resize.
* Several connections with one viewer using tabs.
* Listen mode (Reverse VNC). Via tabs it's possible to listen 
  for and serve multiple incoming connections.
* Record and replay of user input macros.
* Under X11, seamless control of the remote side by moving pointer over the
  (default upper) screen edge. Borrows heavily from x2vnc by
  Fredrik Hübinette <hubbe@hubbe.net>, which in turn was based on
  ideas from x2x and code from vncviewer.
* Simple, loggable statistics.
* MulticastVNC ;-)

### How to get it

[<img src="https://flathub.org/assets/badges/flathub-badge-i-en.png" width="190px" />](https://flathub.org/apps/details/net.christianbeier.MultiVNC)

To get bleeding-edge packages built from the master development branch, navigate to
[the list of CI runs](https://github.com/bk138/multivnc/actions/workflows/ci.yml),
select the last successful one and download the wanted artifact.

### How to build

The prerequisites:

* the usual c-compiler with headers and stuff
* wxWidgets dev package version >= 3.0
* zlib dev package 
* libjpeg dev package 

After cloning the repo, do

```
   git submodule init
   git submodule update
```

To build:

```
   mkdir build
   cd build
   cmake ..
   cmake --build .
   cpack
```

Depending on which OS you are on, you end up with a .deb, .dmg or .exe you can install.

## MulticastVNC notes

You can get a modified libvncserver/libvncclient at
https://github.com/LibVNC/libvncserver/tree/multicastvnc -
this is the same library that MultiVNC uses internally.
