
# I/O Architecture of the Java Version

* MainMenuActivity
  * launches VncCanvasActivity

* VncCanvasActivity
  * contains VNCCanvas, which is a GLSurfaceView
  * contains UI buttons, ZoomControls
  * manages touch input via MightyInputHandler
  * creates a new VNCConn, connects it with the VNCCanvas and sets it up from a ConnectionBean

* VNCConn
  * encapsulates RfbProto
  * facilitates threaded input/output
  * contains one of the framebuffer abstractions and writes to its pixel array and requests
    framebuffer updates using its provided method
  
* InputHandler
  * gets and sets scaling from/on VNCCanvas
  * sets render modes on VNCCanvas 
  * sends touch events to VNCCanvas
  
* VNCCanvas
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
  
   

