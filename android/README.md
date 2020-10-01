# AndroidMultiVNC

## Building the Android Version

To prepare the source tree, do the following in the root of the repository:

* `git submodule update --init`
* `./prepareLibreSSL.sh`

After that setup step, simply fire up Android Studio and build the app from
the `android` directory.

## Technical Notes

### Supported URL Schemes

* `vnc://host:port`

### I/O Architecture of the Java Version

* MainMenuActivity
  * launches VncCanvasActivity

* VncCanvasActivity
  * contains VncCanvas, which is a GLSurfaceView
  * contains UI buttons, ZoomControls
  * contains MightyInputHandler
  * creates a new VNCConn, connects it with the VncCanvas and sets it up from a ConnectionBean

* VNCConn
  * encapsulates RfbProto
  * facilitates threaded input/output
  * contains one of the framebuffer abstractions and writes to its pixel array and requests
    framebuffer updates using its provided method
  
* InputHandler
  * gets and sets scaling from/on VncCanvas
  * sets render modes on VncCanvas 
  * sends touch events to VncCanvas
  
* VncCanvas
  * receives touch events from system, pipes them to InputHandler
  * receives pointer events from InputHandler, pipes them to VNCConn
  * receives key events from VncCanvasActivity, pipes them to VNCConn
  * draws a VNCConn's framebuffer
    * if framebuffer a LargeBitmapData: creates texture from Java Bitmap
    * if framebuffer a FullBufferBitmapData: creates texture from raw buffer data
  * contains some UI as well (Toast and Dialog)
  
* Framebuffer Abstractions
  * LargeBitmapData 
    * is there because of Java memory allocation constraints
    * contains a Bitmap that is smaller than the actual framebuffer
  * FullBufferBitmapData 
    * contains the whole framebuffer as an int[]
  * both request framebuffer updates via RfbProto 
  
   

