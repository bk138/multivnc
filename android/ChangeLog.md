# Version 2.1.x:

## 2.1.8

### 🛠  Fixes

* Fix regression that caused failures when connecting VNC servers using encryption.
* Fix race condition on start when init UI would be tried to show despite activity finishing.

## 2.1.7

### 🛠  Fixes

* Fix another reported crash in screen drawing module.

## 2.1.6

### 🛠  Fixes

* Fix reported crash in screen drawing module.

## 2.1.5

### 🛠  Fixes

* Adapted to API level 34 as required by Google.
* Fixed rare crash in service discovery module.

## 2.1.4

### 🛠  Fixes

* Fixed one rare crash in native code.
* Refactored VncCanvasActivity coupling with other components.
* Fixed server discovery Toast messages wrongly showing when main menu activity was in background.
* Fixed notification permission handling on Android 13+.
* Fixed crash in foreground service showing active connections.

## 2.1.3

### 🛠  Fixes

* Fixed framebuffer update requests being sent although the client being in touchpad mode.
* Updated Android library dependencies.
* Updated libjpeg-turbo to latest ESR release.
* Updated the internal service discovery to use Android-provided machinery, removed dependency on
  jmDNS.
* Refactored coupling of VNC connection handling with UI.

## 2.1.2

### 🛠  Fixes

* Fixed a _server_ crash when connecting to UltraVNC that has MSLogonII enabled.
* Fixed a crash that happened when setting up an SSH tunnel failed for some reason.

## 2.1.1

### 🛠  Fixes

* Fixed bug on Android 7 devices where "VNC connection failed! Only the original thread that created
  a view hierarchy can touch its views." would appear on connection initialisation.

## 2.1.0

This feature release brings a first MVP of SSH-Tunneling, a dark mode, the ability to set
preferred VNC encodings and quality/compress levels and also some bug fixes.

### ⚡ Features

* Added support for night mode aka a dark theme, thanks to Gaurav Ujjwal.
* Added Portuguese and Chinese(Taiwan) translations.
* Improved soft-keyboard access by making keyboard and zoom controls always visible.
* Improved support for physical mice: middle mouse button support, improved right button support.
* Added functionality to set preferred VNC encodings, compress and quality levels, thanks to
  Masato Nagasawa.
* Added a first MVP of SSH-Tunneling: works with password and priv-key.

### 🛠  Fixes

* Fixed a possible crash when trying to (wrongfully) import very big database exports.
* Fixed possible crash when showing connection info.
* Fixed help dialog showing up on 2nd and later connections although the user denied.
* Fixed the haptic feedback of some on-screen actions not obeying the system settings.


# Version 2.0.x:

## 2.0.10

### 🛠  Fixes

* Fix ExtendedDesktopSize handling for older UltraVNC servers.

## 2.0.9

### 🛠  Fixes

* Actually use the new call introduced in v2.0.8.

## 2.0.8

### 🛠  Fixes

* Fixed app crashes on Android >= 11 caused by [Android 11 behavior changes](https://developer.android.com/about/versions/11/behavior-changes-all#fdsan).

## 2.0.7

### 🛠  Fixes

* Fixed keyboard not being toggleable from menu on Android 12.
* Fixed right mouse button clicks from Bluetooth mice being intertwined with flaky left mouse button
  clicks.
* Fixed bookmarks having empty names when saving a connection that was not coming from
  Zeroconf/Bonjour.

## 2.0.6

### 🛠  Fixes

* Fixed connecting to servers running on Fedora 35.
* Fixed white artifacts showing up on Android 12 when scrolling the remote desktop view.
* Fixed the home screen widget so that it now displays all kinds of connections, not only the ones
  with password.

## 2.0.5

### 🛠  Fixes

- Fixed uppercase letters and symbols not working with some VNC servers thanks to Gaurav Ujjwal.

## 2.0.4

### 🛠  Fixes

- Fixed JSON Import/Export in Release builds thanks to Gaurav Ujjwal.
- Fixed Import not showing any files with certain file pickers thanks to Gaurav Ujjwal.
- Fixed crash when copying non-UTF8 characters on the server side, also thanks to Gaurav Ujjwal.

## 2.0.3

### 🛠  Fixes

- Fixed the VNCConnService crash in a different way since the cause was actually a different one. (#172)

## 2.0.2

### 🛠  Fixes

- Fixed Triple-T changelog to simply point to online ChangeLog.md.
- Fixed a race crash in VNCConnService, now in a more robust way.

## 2.0.1

### 🛠  Fixes

- Fixed possible crash on connecting to servers that immediately sent a NewFBSize.
- Fixed a race crash in VNCConnService.

## 2.0.0

Version 2.0.0 is the culmination of the 1.9.x series and marks the completion of
exchanging the previous Java-based RFB engine with a fully native one based on
[LibVNCClient](https://github.com/LibVNC/libvncserver). Here's why:

It's more feature-rich: the native backend now supports Apple Remote Desktop
servers (i.e. all Macs), UltraVNC's MSLogon security type (Microsoft Windows)
and Vino's AnonTLS authentication (GNOME's remote desktop server).

It's faster: with LibVNCClient, MultiVNC can now finally make use of Tight VNC
encoding, a lossy JPEG encoding which drastically reduces needed throughput capacity.
Also, the now-native JPEG decoding can make use of [libjpeg-turbo](https://www.libjpeg-turbo.org/),
a JPEG image codec that uses SIMD instructions to accelerate JPEG decompression.

Big thanks go out to [Gaurav Ujjwal](https://github.com/gujjwal00) who contributed
a lot of very good UI improvements as well as under-the-hood changes. You might want
to check out his excellent [AVNC VNC client for Android](https://github.com/gujjwal00/avnc). 
Thanks also to Alexandr Kondratev, Sergiy Stupar, Suso Comesaña, ferrumcccp,
nagasawa and Frischid for contributing code and translations.

Besides these big changes compared to versions 1.8.x, 2.0.0 brings the following
changes on top of 1.9.11:

### ⚡ Features

- Added handling of remote framebuffer resizes.

### 🛠  Fixes

- Fixed crash on entering wrong password
- Fixed last key-combo not being remembered (#132)
- Fixed import/export on Android 11.

# Version 1.9.x:

## 1.9.11 (2021-02-24)

- Fixed Triple-T metadata.

## 1.9.10 (2021-02-20)

- Fixed connection being terminated when the app was in the background for a longer time.
- Fixed usage over SSH tunnels by not making connections to localhost always use Raw encoding.
- Fixed sending of key combos with modifier keys.
- Fixed jerky cursor movement in high zoom levels.

## 1.9.9 (2021-02-12)

- Added support for handling Samsung SPen button thanks to Frischid.
- Updated internal database handling to modern API thanks to Gaurav Ujjwal.

## 1.9.8 (2021-02-02)

- Made connections to localhost simply use Raw encoding.
- Added screen content resizing on software keyboard opening so keyboard does not obscure screen
  contents thanks to Gaurav Ujjwal.
- Added SIMD support for faster Tight decoding thanks to Gaurav Ujjwal.
- Added Triple-T metadata for F-Droid thanks to Gaurav Ujjwal.
- Added Catalan and Galego translation thanks to Suso Comesaña.
- Improved Japanese translation thanks to Masato Nagasawa.
- Improved Spanish translation thanks to Suso Comesaña.
- Fixed D-Pad focus handling and navigation in the main menu.
- Fixed connection failure when entered server address contained whitespace.
- Fixed crash when importing legacy bookmarks.

## 1.9.7 (2020-10-30)

- Updated Chinese translation.
- Added Spanish and Japanese translations.

## 1.9.6 (2020-10-10)

- Added Ukrainian translation thanks to Sergiy Stupar.
- Removed legacy Java RFB engine, all things VNC are now native. 🎉
- Fixed crash in connection info toast.
- Added functionality to auto-open soft keyboard for credentials dialog.
- Changed Import/Export to use native format, keeping the possibility for legacy XML import.

## 1.9.5 (2020-09-29)

- Made native connections the default, removed chooser dialog.
- Added links to GitHub issues and ChangeLog in About.
- Removed donation links in About for Google Play version.
- Improved russian translation thanks to Alexandr Kondratev.
- Added Chinese (machine) translation.

## 1.9.4 (2020-09-18)

- Reworked UltraVNC-repeater connection functionality to be more user-friendly.
- Fixed right mouse/touchpad button handling for more devices.

## 1.9.3 (2020-08-30)

- Fixed an ANR that occurred when a native connection setup timed out.
- Added colour mode selection for native connections.
- Added functionality to keep screen on when remote screen is shown.

## 1.9.2 (2020-07-17)

- Fixed native connections not working with IPv6.

## 1.9.1 (2020-07-17)

- Added functionality to not ask for frambuffer updates in native-connection touchpad mode.
- Worked around crash in Zeroconf scanner on Android 9.

## 1.9.0 (2020-07-13)

- The first MultiVNC for Android to sport a native backend implementation that supports TightVNC
  and TurboVNC, Apple Remote Desktop and Vino's AnonTLS authentication. In the 1.9.x builds this
  will start as an opt-in feature. 1.9.x builds will be available via the Google Play Beta program
  or F-Droid only.
- Improved scaling so the whole remote screen can be seen at once in portrait orientation.
- Improved the credentials dialog to provide more hints on what is being entered in landscape mode.

# Version 1.8.x:

## 1.8.12 (2020-07-16)

- Added dialog notifying about beta test program.
- Worked around crash in Zeroconf scanner on Android 9.

## 1.8.11 (2020-07-02)

- The whole screen scaling user experience is now way smoother thanks to Gaurav Ujjwal.
- The online help text was updated.
- MultiVNC now requires Android 5 in order to make better use of vector drawables.

## 1.8.10 (2020-06-07)

- Added support for external mouse's scroll wheels.
- MultiVNC now completely uses vector images.

## 1.8.9 (2020-03-01)

- VNC activity is now rotatable thanks to work done by Gaurav Ujjwal.
- Added ability to make key combos with 'Super' key.
- Turned more icons into vector icons.
- Fixed some possible crashes.

## 1.8.8 (2019-12-31)

- Now hides navigation bar per default, resulting in more usable screen space.
- Fixed hamburger menu not opening on some Samsung devices.

## 1.8.7 (2019-12-18)

- Fixed possible crash in import/export activity.

## 1.8.6 (2019-11-25)

- Added ability to connect to MacOS servers.
- Fixed bookmarks sometimes not having a name.

## 1.8.5 (2019-11-17)

- Handle right-mouse-button-clicks from USB-OTG mice the right way.

## 1.8.4 (2019-10-27)

- Fix erronous auto-bookmarking of connections started via vnc:// scheme.
- Fix some keyboards not accepting input.
- Fix resending of sent special key combo.

## 1.8.3 (2019-10-17)

- Main menu layout improved on small tablet devices.
- Fixed two possible crashes.

## 1.8.2 (2019-09-29)

- Some UI fixes for Android Kitkat.

## 1.8.1 (2019-09-16)

- More adaptations for tablets.
- Fixed two possible crashes.

## 1.8.0 (2019-09-14)

- Updated UI to use Material Design.
- Made remote desktop view use more screen real estate.
- Changed default color mode to TrueColor.
- Fixed possible crash on app start.

# Version 1.7.x:

## 1.7.10 (2019-09-04)

- Fixed a possible race condition and subsequent crash on app start.

## 1.7.9 (2019-07-31)

- Fixed a possible crash.
- Added Russian translation.

## 1.7.8 (2019-07-06)

- Added Italian translation.

## 1.7.7 (2019-06-30)

- Fixed possible crash on ChromeBox.

## 1.7.6 (2019-04-07)

- Inform user when there are no bookmarks to create shortcuts with.
- Fixed possible crash when importing settings.

## 1.7.5 (2019-01-15)

- Fixed possible crash when closing the app.

## 1.7.4 (2019-01-13)

- Fixed possible crash in cursor handling.

## 1.7.3 (2019-01-12)

- Some updates to the server discovery module.
- Fixed minor issues in german translation.

## 1.7.2 (2018-12-07)

- Fixed another possible crash.
- Added german translation.

## 1.7.1 (2018-12-02)

- Fix crash on connection initialisation.

## 1.7.0 (2018-11-18)

- Updated the UI theming to use the default look on the respective Android version.
- Made the import/export UI more usable.
- Updated the launcher icon to be more hi-res.
- Removed the ad system.

# Version 1.6.x:

## 1.6.4 (2014-01-07)

- Fix immediate crash upon connecting on some devices.

## 1.6.3 (2013-06-27)

- Fix hidden menu items on some devices.
- Fix Zeroconf/Bonjour/mDNS VNC server discovery on Android 4.2 devices.

## 1.6.2 (2013-06-06)

- Hide Actionbar when there is a hardware menu button.
- Removed broken hungarian translation.

## 1.6.1 (2013-03-11)

- Fixed ANR that could sometimes occur when connection broke.
- Fixed crash in discovered servers list.

## 1.6.0 (2013-02-03)

- Added support for hardware keyboards and mice. Kudos to Kevin Pansky.
- Added copy&paste function to and from Android clipboard.
- Virtual mouse button click-and-drag now also works on Honeycomb and newer devices.
- A bit more screen real estate on ICS and newer devices by hiding the status bar.

# Version 1.5.x:

## 1.5.2 (2012-12-26)

- Hopefully fix crash on dialog dismissal.

## 1.5.1 (2012-12-18)

- Prevent ActionBar from stealing keyboard focus.

## 1.5.0 (2012-12-14)

- Full IPv6 support on Honeycomb and newer!
- Added help screen.
- Improved touchpad mode.
- Fixed 'Key Combo' button being focused when soft keyboard was activated.
- Fixed virtual mouse button toggling.
- Fixed bookmarked connections.
- Fixed ActionBar taking focus on soft keyboard toggle.
- Fixed a lot of maybe-crash situations.
- Removed broken Flattr button.

# Version 1.4.x:

## 1.4.10 (2012-08-23)

- Added button to be able to rate the app.

## 1.4.9 (2012-08-13)

- Fix crash introduced by the 1.4.8 fix.

## 1.4.8 (2012-08-12)

- Fix an ANR error when pausing the app.

## 1.4.7 (2012-07-15)

- Fix hang after password entry. Should also work with bookmarks now.

## 1.4.6 (2012-07-14)

- Add menu item to toggle pointer highlighting.

## 1.4.5 (2012-06-26)

- Fix the 1.4.4 fix.

## 1.4.4 (2012-06-23)

- Fix crash on startup on Honeycomb or newer devices.

## 1.4.3 (2012-06-18)

- Hopefully fixed keyboard disappearing bug.

## 1.4.2 (2012-06-15)

- Fixed crash on mouse button press.

## 1.4.1 (2012-06-14)

- Fixed app not starting on Honeycomb devices.

## 1.4.0 (2012-06-12)

- ICS and Tablet Support!
- Added password prompt. Shown when no password was given but one is required.
- Greatly improved key handling. Most special symbols work now.
- Fixed a nasty app restart bug on Honeycomb and newer devices.
- Fixed some possible crashes.
