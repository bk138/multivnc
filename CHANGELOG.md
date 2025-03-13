# MultiVNC 0.10.0 - "Spring Break" - 2025-03-13
* Fixed hangs that could occur when connections were interrupted.
* Improved connection setup dialog.
* Fixed warning dialog appearing when English fallback locale was used on MacOS.
* Added ability to connect to UltraVNC-style Mode2 repeaters.

# MultiVNC 0.9.0 - "Hello New Year" - 2024-12-29
* Improved bookmark editing/deleting by adding a context menu.
* Added UTF-8 cuttext handling.
* Improved log window by making it wider.
* Fixed a bug where MultiVNC on Apple Silicon would not be able to connect
  to TLS-enabled servers.

# MultiVNC 0.8.0 - "Hello New World" - 2024-12-06
* Added a scale-to-fit view mode which is now the default.
  Previous 1-to-1 view mode can still be toggled.
* Improved fullscreen mode to show less controls and more of the remote view.
* Improved toolbar icons to use scalable resources that look good for any
  display resolution as well as light and dark display mode.
* Added MacOS packaging.

# MultiVNC 0.7.0 - "A step in between" - 2023-11-05
* Added Swedish translation thanks to Åke Engelbrektson.
* Added more tooltips to more UI elements.
* Added keyboard shortcut for making a new connection.
* Added secret store use for credentials without user name.
* Fixed error dialog sometimes being not shown.
* Fixed drawing on MacOS and Wayland. The flatpak now uses Wayland.
* Fixed hang when connecting to unreachable servers.

# MultiVNC 0.6.0 - "Back from the dead" - 2023-05-17
* Added record/replay of user input. You can now record a
  VNC session and replay this 'macro' later.
* Added support for entering credentials on login and saving them
  in bookmarks.
* Added translations into German and Spanish.
* Added flatpak packaging.
* Added easy emailing of bug reports.
* Added optional OpenSSL support (instead of GnuTLS).
* Added ability to listen on IPv6 addresses.
* Added and fixed Linux, MacOS and Windows continous integration.
* Fixed the portable edition for Windows: it is now truly
  portable as it saves its preferences into a file under Windows
  now, and is not using the registry.

# MultiVNC 0.5 - "Bag o' stuff" - 2011-05-07
* MulticastVNC now supports the Ultra encoding and features
  a freely sizeable receive buffer in addition to the 
  OS-dependent socket receive buffer.
* The new MulticastVNC flow control adapts the server's
  transmission rate to the capabilities of the network
  and clients.
* MultiVNC is now able to connect to Apple Remote Desktop
  servers.
* It is now possible to select the desired VNC encoding.
* Encrypted connections now work on Windows as well.
* Added a keyboard grab button that allows to enter arbitrary
  key combinations like Ctrl-Alt-Del into MultiVNC without
  being interpreted by the local OS. Works on UNIX by now.
* This time really fixed the bug with the viewer becoming
  unresponsive under high multicast loads.
* Fixed not being able to enter IPv6 addresses.

# MultiVNC 0.4.1 - "Fix me up" - 2010-11-09
* Changed default MulticastVNC receive buffer size to 5120 KB
  to play it safe.
* Shared windows are now view-only on Linux clients. Having 
  the local cursor warped here and there is quite distracting.
* Window sharing on Windows has no more trayicon and 
  distracting setup dialog.
* Window sharing should be more stable now.
* Link latency measuring is way more exact now.
* Fixed crash on startup when toolbar was disabled.
* Fixed Windows build. MultiVNC 0.4 was unusable.

# MultiVNC 0.4 - "One for the road" - 2010-10-27
* MultiVNC can now connect to IPv6 servers!
* MulticastVNC now incorporates a NACK mechanism which makes
  multicasting a lot more reliable: Lost packets are 
  retransmitted if the server supports it.
* MultiVNC now supports QoS: data sent to a VNC server can be
  marked for expedited forwarding!
* Fixed bug where changing edge connector settings would
  crash MultiVNC.

# MultiVNC 0.3 - "Collab me!" - 2010-07-05
* Implemented window sharing: You can now beam one of your 
  windows to the remote side. Works on UNIX and Windows.
* Implemented Edge Connector: You can now tell MultiVNC that
  one of your screen edges should be the border to the remote
  display. When you move the pointer over this edge, MultiVNC
  will take over your mouse and keyboard and send your input 
  to the remote side. When the pointer is moved back towards 
  the opposite edge on the remote screen, you're back on your
  local desktop.
* Implemented FastRequest feature: You can now set an interval
  at which MultiVNC continously asks the server for updates
  instead of just asking after each received server message.
  This should improve framerate on high latency links.
* Fixed a bug where a fast VNC server would make MultiVNC
  GUI unresponsive on a slow client.
* Fixed bug where saving log or screenshot would terminate
  the connection or even crash on some machines.
* Fixed bug where setting up connection would take very long
  due to unnecessary DNS lookups.


# MultiVNC 0.2 - "The real thing" - 2010-02-15
* MulticastVNC incorporated! You can now connect to a
  MulticastVNC enabled server, e.g. based on libvncserver.
* Listen mode (Reverse VNC) implemented. Via tabs it is
  possible to listen for and serve multiple incoming
  connections.
* Bookmarks are working now.
* Clipboard data is now properly encoded.
* Fix bug with canvas centering.
* Fix bug where disconnecting from a fast server would 
  terminate MultiVNC.
* Fix really nasty bug where listening on windows would
  take 100% CPU.


# MultiVNC 0.1 - "In the beginning" - 2009-10-12
* a usable tight-enabled vncviewer with the following special
  features:
* supports server framebuffer resize
* several connections with one viewer using tabs
* simple, loggable statistics
* discovery of VNC servers advertising themselves via ZeroConf
* runs under UNIX and Windows

