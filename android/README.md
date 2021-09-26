# AndroidMultiVNC

[<img src="https://fdroid.gitlab.io/artwork/badge/get-it-on.png"
     alt="Get it on F-Droid"
     height="80">](https://f-droid.org/packages/com.coboltforge.dontmind.multivnc/)
[<img src="https://play.google.com/intl/en_us/badges/images/generic/en-play-badge.png"
     alt="Get it on Google Play"
     height="80">](https://play.google.com/store/apps/details?id=com.coboltforge.dontmind.multivnc)

## Building the Android Version

To prepare the source tree, do the following in the root of the repository:

* `git submodule update --init`
* `./prepareLibreSSL.sh`

After that setup step, simply fire up Android Studio and build the app from
the `android` directory.

## Technical Notes

### Supported URL Schemes

* `vnc://host:port`

### I/O Architecture

* MainMenuActivity
  * launches VncCanvasActivity

* VncCanvasActivity
  * contains VncCanvas, which is a GLSurfaceView
  * contains UI buttons, ZoomControls
  * contains MightyInputHandler
  * creates a new VNCConn, connects it with the VncCanvas and sets it up from a ConnectionBean

* VNCConn
  * encapsulates the native rfbClient, allows querying info from it as well as triggering actions
  * facilitates threaded input/output
  
* InputHandler
  * gets scaling from VncCanvas
  * sends pointer events (from touch, gestures, mouse, stylus) to VncCanvas
  
* VncCanvas
  * receives touch/motion events from system, pipes them to InputHandler
  * receives pointer events from InputHandler, pipes them to VNCConn
  * receives key events from VncCanvasActivity, pipes them to VNCConn
  * draws a VNCConn's rfbClients's framebuffer
    * OpenGL setup and drawing is done in managed code
    * framebuffer data is bound to texture by native code
  * contains some UI as well (Toast and Dialog)
  
* Framebuffer Abstractions
  * none, framebuffer completely resides in native part
  
   

