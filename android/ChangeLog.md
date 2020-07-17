# Version 1.9.x:

1.9.2

-   Fixed native connections not working with IPv6.

1.9.1

-   Added functionality to not ask for frambuffer updates in native-connection touchpad mode.
-   Worked around crash in Zeroconf scanner on Android 9.

1.9.0

-   The first MultiVNC for Android to sport a native backend implementation that
    supports TightVNC and TurboVNC, Apple Remote Desktop and Vino's AnonTLS authentication.
    In the 1.9.x builds this will be an opt-in feature. 1.9.x builds will be available via
    the Google Play Beta program or F-Droid only.
-   Improved scaling so the whole remote screen can be seen at once in portrait orientation.
-   Improved the credentials dialog to provide more hints on what is being entered in
    landscape mode.

# Version 1.8.x:

1.8.12

-   Added dialog notifying about beta test program.
-   Worked around crash in Zeroconf scanner on Android 9.

1.8.11

-   The whole screen scaling user experience is now way smoother thanks to Gaurav Ujjwal.
-   The online help text was updated.
-   MultiVNC now requires Android 5 in order to make better use of vector drawables.

1.8.10

-   Added support for external mouse's scroll wheels.
-   MultiVNC now completely uses vector images.

1.8.9

-   VNC activity is now rotatable thanks to work done by Gaurav Ujjwal.
-   Added ability to make key combos with 'Super' key.
-   Turned more icons into vector icons.
-   Fixed some possible crashes.

1.8.8

-   Now hides navigation bar per default, resulting in more usable screen space.
-   Fixed hamburger menu not opening on some Samsung devices.

1.8.7

-   Fixed possible crash in import/export activity.

1.8.6

-   Added ability to connect to MacOS servers.
-   Fixed bookmarks sometimes not having a name.

1.8.5

-   Handle right-mouse-button-clicks from USB-OTG mice the right way.

1.8.4

-   Fix erronous auto-bookmarking of connections started via vnc:// scheme.
-   Fix some keyboards not accepting input.
-   Fix resending of sent special key combo.

1.8.3

-   Main menu layout improved on small tablet devices.
-   Fixed two possible crashes.

1.8.2

-   Some UI fixes for Android Kitkat.

1.8.1

-   More adaptations for tablets.
-   Fixed two possible crashes.

1.8.0

-   Updated UI to use Material Design.
-   Made remote desktop view use more screen real estate.
-   Changed default color mode to TrueColor.
-   Fixed possible crash on app start.

# Version 1.7.x:

1.7.10

-   Fixed a possible race condition and subsequent crash on app start.

1.7.9

-   Fixed a possible crash.
-   Added Russian translation.

1.7.8

-   Added Italian translation.

1.7.7

-   Fixed possible crash on ChromeBox.

1.7.6

-   Inform user when there are no bookmarks to create shortcuts with.
-   Fixed possible crash when importing settings.

1.7.5

-   Fixed possible crash when closing the app.

1.7.4

-   Fixed possible crash in cursor handling.

1.7.3

-   Some updates to the server discovery module.
-   Fixed minor issues in german translation.

1.7.2

-   Fixed another possible crash.
-   Added german translation.

1.7.1

-   Fix crash on connection initialisation.

1.7.0

-   Updated the UI theming to use the default look on the respective
    Android version.
-   Made the import/export UI more usable.
-   Updated the launcher icon to be more hi-res.
-   Removed the ad system.

# Version 1.6.x:

1.6.4

-   Fix immediate crash upon connecting on some devices.

1.6.3

-   Fix hidden menu items on some devices.
-   Fix Zeroconf/Bonjour/mDNS VNC server discovery on Android 4.2
    devices.

1.6.2

-   Hide Actionbar when there is a hardware menu button.
-   Removed broken hungarian translation.

1.6.1

-   Fixed ANR that could sometimes occur when connection broke.
-   Fixed crash in discovered servers list.

1.6.0

-   Added support for hardware keyboards and mice. Kudos to Kevin
    Pansky.
-   Added copy&paste function to and from Android clipboard.
-   Virtual mouse button click-and-drag now also works on Honeycomb and
    newer devices.
-   A bit more screen real estate on ICS and newer devices by hiding the
    status bar.

# Version 1.5.x:

1.5.2

-   Hopefully fix crash on dialog dismissal.

1.5.1

-   Prevent ActionBar from stealing keyboard focus.

1.5.0

-   Full IPv6 support on Honeycomb and newer!
-   Added help screen.
-   Improved touchpad mode.
-   Fixed \\\'Key Combo\\\' button being focused when soft keyboard was
    activated.
-   Fixed virtual mouse button toggling.
-   Fixed bookmarked connections.
-   Fixed ActionBar taking focus on soft keyboard toggle.
-   Fixed a lot of maybe-crash situations.
-   Removed broken flattr button.

# Version 1.4.x:

1.4.10

-   Added button to be able to rate the app.

1.4.9

-   Fix crash introduced by the 1.4.8 fix.

1.4.8

-   Fix an ANR error when pausing the app.

1.4.7

-   Fix hang after password entry. Should also work with bookmarks now.

1.4.6

-   Add menu item to toggle pointer highlighting.

1.4.5

-   Fix the 1.4.4 fix.

1.4.4

-   Fix crash on startup on Honeycomb or newer devices.

1.4.3

-   Hopefully fixed keyboard disappearing bug.

1.4.2

-   Fixed crash on mouse button press.

1.4.1

-   Fixed app not starting on Honeycomb devices.

1.4.0

-   ICS and Tablet Support!
-   Added password prompt. Shown when no password was given but one is
    required.
-   Greatly improved key handling. Most special symbols work now.
-   Fixed a nasty app restart bug on Honeycomb and newer devices.
-   Fixed some possible crashes.
