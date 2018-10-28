/**
 *
 * This represents an *active* VNC connection (as opposed to ConnectionBean, which is more like a bookmark.).
 *
 * Copyright (C) 2012 Christian Beier <dontmind@freeshell.org>
 */


package com.coboltforge.dontmind.multivnc;

import java.io.IOException;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import java.util.zip.Inflater;

import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.Paint.Style;
import android.util.Log;
import android.view.KeyEvent;
import android.widget.Toast;



public class VNCConn {

	private final static String TAG = "VNCConn";

	private VncCanvas canvas;

	private VncInputThread inputThread;
	private VNCOutputThread outputThread;

	// VNC protocol connection
	private RfbProto rfb;
	private ConnectionBean connSettings;
	private COLORMODEL pendingColorModel = COLORMODEL.C24bit;

	// Runtime control flags
	private boolean maintainConnection = true;
	private boolean framebufferUpdatesEnabled = true;

	// Internal bitmap data
	private AbstractBitmapData bitmapData;
	private Lock bitmapDataPixelsLock = new ReentrantLock();

	// message queue for communicating with the output worker thread
	private ConcurrentLinkedQueue<OutputEvent> outputEventQueue = new ConcurrentLinkedQueue<VNCConn.OutputEvent>();

	private Paint handleRREPaint;

	// ZRLE encoder's data.
	private byte[] zrleBuf;
	private int[] zrleTilePixels;
	private ZlibInStream zrleInStream;

	// Zlib encoder's data.
	private byte[] zlibBuf;
	private Inflater zlibInflater;


	private int bytesPerPixel;
	private int[] colorPalette;
	private COLORMODEL colorModel;

	private String serverCutText;

	// VNC Encoding parameters
	private boolean useCopyRect = false; // TODO CopyRect is not working
	private int preferredEncoding = -1;

	// Unimplemented TIGHT encoding parameters
	private int compressLevel = -1;
	private int jpegQuality = -1;

	// Used to determine if encoding update is necessary
	private int[] encodingsSaved = new int[20];
	private int nEncodingsSaved = 0;

	// Useful shortcuts for modifier masks.
    public final static int CTRL_MASK  = KeyEvent.META_SYM_ON;
    public final static int SHIFT_MASK = KeyEvent.META_SHIFT_ON;
    public final static int META_MASK  = 0;
    public final static int ALT_MASK   = KeyEvent.META_ALT_ON;

    public static final int MOUSE_BUTTON_NONE = 0;
    public static final int MOUSE_BUTTON_LEFT = 1;
    public static final int MOUSE_BUTTON_MIDDLE = 2;
    public static final int MOUSE_BUTTON_RIGHT = 4;
    public static final int MOUSE_BUTTON_SCROLL_UP = 8;
    public static final int MOUSE_BUTTON_SCROLL_DOWN = 16;


    private class OutputEvent {

    	public OutputEvent(int x, int y, int modifiers, int pointerMask) {
    		pointer = new PointerEvent();
    		pointer.x = x;
    		pointer.y = y;
    		pointer.modifiers = modifiers;
    		pointer.mask = pointerMask;
    	}

    	public OutputEvent(int keyCode, int metaState, boolean down) {
    		key = new KeyboardEvent();
    		key.keyCode = keyCode;
    		key.metaState = metaState;
    		key.down = down;
    	}

    	public OutputEvent(boolean incremental) {
    		ffur = new FullFramebufferUpdateRequest();
    		ffur.incremental = incremental;
    	}

    	public OutputEvent(String text) {
    		cuttext = new ClientCutText();
    		cuttext.text = text;
    	}

    	private class PointerEvent {
    		int x;
    		int y;
    		int mask;
    		int modifiers;
    	}

    	private class KeyboardEvent {
    		int keyCode;
    		int metaState;
    		boolean down;
    	}

    	private class FullFramebufferUpdateRequest {
    		boolean incremental;
    	}

    	private class ClientCutText {
    		String text;
    	}

    	public FullFramebufferUpdateRequest ffur;
    	public PointerEvent pointer;
    	public KeyboardEvent key;
    	public ClientCutText cuttext;
    }


    private class VncInputThread extends Thread {

    	private ProgressDialog pd;
    	private Runnable setModes;


    	public VncInputThread(ProgressDialog pd, Runnable setModes) {
    		this.pd = pd;
    		this.setModes = setModes;
    	}


		public void run() {

			if(Utils.DEBUG()) Log.d(TAG, "InputThread started!");

			try {
				connectAndAuthenticate();
				doProtocolInitialisation(canvas.getWidth(), canvas.getHeight());
				canvas.handler.post(new Runnable() {
					public void run() {
						canvas.activity.setTitle(getDesktopName());
						pd.setMessage("Downloading first frame.\nPlease wait...");
					}
				});

				// start output thread here
				outputThread = new VNCOutputThread();
				outputThread.start();

				processNormalProtocol(canvas.getContext(), pd, setModes);
			} catch (Throwable e) {
				if (maintainConnection) {
					Log.e(TAG, e.toString());
					e.printStackTrace();
					// Ensure we dismiss the progress dialog
					// before we fatal error finish
					canvas.handler.post(new Runnable() {
						public void run() {
							if(pd.isShowing())
								pd.dismiss();
						}
					});
					if (e instanceof OutOfMemoryError) {
						// TODO  Not sure if this will happen but...
						// figure out how to gracefully notify the user
						// Instantiating an alert dialog here doesn't work
						// because we are out of memory. :(
					} else {
						String error = "VNC connection failed!";
						if (e.getMessage() != null && (e.getMessage().indexOf("authentication") > -1)) {
							error = "VNC authentication failed!";
						}
						final String error_ = error + "<br>" + e.getLocalizedMessage();
						canvas.handler.post(new Runnable() {
							public void run() {
								Utils.showFatalErrorMessage(canvas.getContext(), error_);
							}
						});
					}
				}
			}

			if(Utils.DEBUG()) Log.d(TAG, "InputThread done!");
		}



		private void connectAndAuthenticate() throws Exception {

			/*
		     * if IPv6 address, add scope id
		     */
		    try {
				InetAddress address = InetAddress.getByName(connSettings.getAddress());

				Inet6Address in6 = Inet6Address.getByAddress(
						address.getHostName(),
						address.getAddress(),
						Utils.getActiveNetworkInterface(canvas.getContext()));

				connSettings.setAddress(in6.getHostAddress());
				Log.i(TAG, "Using IPv6");

			} catch (UnknownHostException e) {
				Log.i(TAG, "Using IPv4: " + e.toString());
			} catch (NullPointerException ne) {
				Log.e(TAG, ne.toString());
			}



			Log.i(TAG, "Connecting to " + connSettings.getAddress() + ", port " + connSettings.getPort() + "...");

			rfb = new RfbProto(connSettings.getAddress(), connSettings.getPort());
			Log.v(TAG, "Connected to server");

			// <RepeaterMagic>
			if (connSettings.getUseRepeater() && connSettings.getRepeaterId() != null && connSettings.getRepeaterId().length()>0) {
				Log.i(TAG, "Negotiating repeater/proxy connSettings");
				byte[] protocolMsg = new byte[12];
				rfb.is.read(protocolMsg);
				byte[] buffer = new byte[250];
				System.arraycopy(connSettings.getRepeaterId().getBytes(), 0, buffer, 0, connSettings.getRepeaterId().length());
				rfb.os.write(buffer);
			}
			// </RepeaterMagic>

			rfb.readVersionMsg();
			Log.i(TAG, "RFB server supports protocol version " + rfb.serverMajor + "." + rfb.serverMinor);

			rfb.writeVersionMsg();
			Log.i(TAG, "Using RFB protocol version " + rfb.clientMajor + "." + rfb.clientMinor);

			int bitPref=0;
			if(connSettings.getUserName().length()>0)
				bitPref|=1;
			if(Utils.DEBUG()) Log.d("debug","bitPref="+bitPref);
			int secType = rfb.negotiateSecurity(bitPref);
			int authType;
			if (secType == RfbProto.SecTypeTight) {
				rfb.initCapabilities();
				rfb.setupTunneling();
				authType = rfb.negotiateAuthenticationTight();
			} else if (secType == RfbProto.SecTypeUltra34) {
				rfb.prepareDH();
				authType = RfbProto.AuthUltra;
			} else {
				authType = secType;
			}

			switch (authType) {
			case RfbProto.AuthNone:
				Log.i(TAG, "No authentication needed");
				rfb.authenticateNone();
				break;
			case RfbProto.AuthVNC:
				Log.i(TAG, "VNC authentication needed");
				if(connSettings.getPassword() == null || connSettings.getPassword().length() == 0) {
					canvas.getCredFromUser(connSettings);
					synchronized (VNCConn.this) {
						VNCConn.this.wait();  // wait for user input to finish
					}
				}
				rfb.authenticateVNC(connSettings.getPassword());
				break;
			case RfbProto.AuthUltra:
				if(connSettings.getPassword() == null || connSettings.getPassword().length() == 0) {
					canvas.getCredFromUser(connSettings);
					synchronized (VNCConn.this) {
						VNCConn.this.wait();  // wait for user input to finish
					}
				}
				rfb.authenticateDH(connSettings.getUserName(),connSettings.getPassword());
				break;
			default:
				throw new Exception("Unknown authentication scheme " + authType);
			}
		}



		private void doProtocolInitialisation(int dx, int dy) throws IOException {
			rfb.writeClientInit();
			rfb.readServerInit();

			Log.i(TAG, "Desktop name is " + rfb.desktopName);
			Log.i(TAG, "Desktop size is " + rfb.framebufferWidth + " x " + rfb.framebufferHeight);

			canvas.mouseX = rfb.framebufferWidth/2;
			canvas.mouseY = rfb.framebufferHeight/2;

			boolean useFull = false;
			int capacity = Utils.getActivityManager(canvas.getContext()).getMemoryClass();
			if (connSettings.getForceFull() == BitmapImplHint.AUTO)
			{
				if (rfb.framebufferWidth * rfb.framebufferHeight * FullBufferBitmapData.CAPACITY_MULTIPLIER <= capacity * 1024 * 1024)
					useFull = true;
			}
			else
				useFull = (connSettings.getForceFull() == BitmapImplHint.FULL);
			if (! useFull)
				bitmapData=new LargeBitmapData(rfb, canvas, capacity);
			else
				bitmapData=new FullBufferBitmapData(rfb, canvas, capacity);

			setPixelFormat();
		}


		private void processNormalProtocol(final Context context, final ProgressDialog pd, final Runnable setModes) throws Exception {
			try {

				Log.d(TAG, "Connection initialized");

				bitmapData.writeFullUpdateRequest(false);

				canvas.handler.post(setModes);

				//
				// main input loop
				//
				while (maintainConnection) {

					bitmapData.syncScroll();
					// Read message type from the server.
					int msgType = rfb.readServerMessageType();
					bitmapData.doneWaiting();
					// Process the message depending on its type.
					switch (msgType) {
					case RfbProto.FramebufferUpdate:
						rfb.readFramebufferUpdate();

						for (int i = 0; i < rfb.updateNRects; i++) {
							rfb.readFramebufferUpdateRectHdr();
							int rx = rfb.updateRectX, ry = rfb.updateRectY;
							int rw = rfb.updateRectW, rh = rfb.updateRectH;

							if (rfb.updateRectEncoding == RfbProto.EncodingLastRect) {
								Log.v(TAG, "rfb.EncodingLastRect");
								break;
							}

							if (rfb.updateRectEncoding == RfbProto.EncodingNewFBSize) {
								rfb.setFramebufferSize(rw, rh);
								// - updateFramebufferSize();
								Log.v(TAG, "rfb.EncodingNewFBSize");
								break;
							}

							if (rfb.updateRectEncoding == RfbProto.EncodingXCursor || rfb.updateRectEncoding == RfbProto.EncodingRichCursor) {
								// - handleCursorShapeUpdate(rfb.updateRectEncoding,
								// rx,
								// ry, rw, rh);
								Log.v(TAG, "rfb.EncodingCursor");
								continue;

							}

							if (rfb.updateRectEncoding == RfbProto.EncodingPointerPos) {
								canvas.mouseX = rx;
								canvas.mouseY = ry;
								continue;
							}

							rfb.startTiming();

							switch (rfb.updateRectEncoding) {
							case RfbProto.EncodingRaw:
								handleRawRect(rx, ry, rw, rh);
								break;
							case RfbProto.EncodingCopyRect:
								handleCopyRect(rx, ry, rw, rh);
								Log.v(TAG, "CopyRect is Buggy!");
								break;
							case RfbProto.EncodingRRE:
								handleRRERect(rx, ry, rw, rh);
								break;
							case RfbProto.EncodingCoRRE:
								handleCoRRERect(rx, ry, rw, rh);
								break;
							case RfbProto.EncodingHextile:
								handleHextileRect(rx, ry, rw, rh);
								break;
							case RfbProto.EncodingZRLE:
								handleZRLERect(rx, ry, rw, rh);
								break;
							case RfbProto.EncodingZlib:
								handleZlibRect(rx, ry, rw, rh);
								break;
							default:
								Log.e(TAG, "Unknown RFB rectangle encoding " + rfb.updateRectEncoding + " (0x" + Integer.toHexString(rfb.updateRectEncoding) + ")");
							}

							rfb.stopTiming();

							// Hide progress dialog
							canvas.handler.post(new Runnable() {
								public void run() {
									if(pd.isShowing())
										pd.dismiss();
								}
							});
						}

						boolean fullUpdateNeeded = false;

						if (pendingColorModel != null) {
							setPixelFormat();
							fullUpdateNeeded = true;
						}

						setEncodings(true);
						if(framebufferUpdatesEnabled)
							bitmapData.writeFullUpdateRequest(!fullUpdateNeeded);

						break;

					case RfbProto.SetColourMapEntries:
						throw new Exception("Can't handle SetColourMapEntries message");

					case RfbProto.Bell:
						canvas.handler.post( new Runnable() {
							public void run() { Toast.makeText( context, "VNC Beep", Toast.LENGTH_SHORT); }
						});
						break;

					case RfbProto.ServerCutText:
						serverCutText = rfb.readServerCutText();
						Log.d(TAG, "got server cuttext: " + serverCutText);
						break;

					case RfbProto.TextChat:
						// UltraVNC extension
						String msg = rfb.readTextChatMsg();
						if (msg != null && msg.length() > 0) {
							// TODO implement chat interface
						}
						break;

					default:
						throw new Exception("Unknown RFB message type " + msgType);
					}
				}
			} catch (Exception e) {
				throw e;
			} finally {

				// unlock this lock, probably was held when IO exception occured
				try {
					bitmapDataPixelsLock.unlock();
				}
				catch(Exception e) {
				}

				Log.v(TAG, "Closing VNC Connection");
				rfb.close();
				System.gc();
			}
		}


		private void setPixelFormat() throws IOException {
			if(bitmapData instanceof FullBufferBitmapData)
				setPixelFormatFromModel(pendingColorModel, true);
			else
				setPixelFormatFromModel(pendingColorModel, false);
			bytesPerPixel = pendingColorModel.bpp();
			colorPalette = pendingColorModel.palette();
			colorModel = pendingColorModel;
			pendingColorModel = null;
		}



		private void setPixelFormatFromModel(COLORMODEL model, boolean reverserPixelOrder) throws IOException {
			switch (model) {
			case C24bit:
				// 24-bit color
				if(reverserPixelOrder)
					rfb.writeSetPixelFormat(32, 24, false, true, 255, 255, 255, 0, 8, 16, false);
				else
					rfb.writeSetPixelFormat(32, 24, false, true, 255, 255, 255, 16, 8, 0, false);
				break;
			case C256:
				if(reverserPixelOrder)
					rfb.writeSetPixelFormat(8, 8, false, true, 3, 7, 7, 6, 3, 0, false); // not ideal, but wtf...
				else
					rfb.writeSetPixelFormat(8, 8, false, true, 7, 7, 3, 0, 3, 6, false);
				break;
			case C64:
				if(reverserPixelOrder)
					rfb.writeSetPixelFormat(8, 6, false, true, 3, 3, 3, 0, 2, 4, false);
				else
					rfb.writeSetPixelFormat(8, 6, false, true, 3, 3, 3, 4, 2, 0, false);
				break;
			case C8:
				if(reverserPixelOrder)
					rfb.writeSetPixelFormat(8, 3, false, true, 1, 1, 1, 0, 1, 2, false);
				else
					rfb.writeSetPixelFormat(8, 3, false, true, 1, 1, 1, 2, 1, 0, false);
				break;
			case C4:
				// Greyscale
				if(reverserPixelOrder)
					rfb.writeSetPixelFormat(8, 6, false, true, 3, 3, 3, 0, 2, 4, true);
				else
					rfb.writeSetPixelFormat(8, 6, false, true, 3, 3, 3, 4, 2, 0, true);
				break;
			case C2:
				// B&W
				if(reverserPixelOrder)
					rfb.writeSetPixelFormat(8, 3, false, true, 1, 1, 1, 0, 1, 2, true);
				else
					rfb.writeSetPixelFormat(8, 3, false, true, 1, 1, 1, 2, 1, 0, true);
				break;
			default:
				// Default is 24bit colors
				if(reverserPixelOrder)
					rfb.writeSetPixelFormat(32, 24, false, true, 255, 255, 255, 0, 8, 16, false);
				else
					rfb.writeSetPixelFormat(32, 24, false, true, 255, 255, 255, 16, 8, 0, false);
				break;
			}
		}





    }




    private class VNCOutputThread extends Thread {

    	public void run() {

			if(Utils.DEBUG()) Log.d(TAG, "OutputThread started!");

    		//
    		// main output loop
    		//
    		while (maintainConnection) {

    			// check input queue
    			OutputEvent ev;
    			while( (ev = outputEventQueue.poll()) != null ) {
    				if(ev.pointer != null)
    					sendPointerEvent(ev.pointer);
    				if(ev.key != null)
    					sendKeyEvent(ev.key);
    				if(ev.ffur != null)
						try {
							bitmapData.writeFullUpdateRequest(ev.ffur.incremental);
						} catch (IOException e) {
							e.printStackTrace();
						}
    				if(ev.cuttext != null)
    					sendCutText(ev.cuttext.text);
    			}

    			// at this point, queue is empty, wait for input instead of hogging CPU
    			synchronized (outputEventQueue) {
					try {
						outputEventQueue.wait();
					} catch (InterruptedException e) {
						// go on
					}
				}

    		}

    		if(Utils.DEBUG()) Log.d(TAG, "OutputThread done!");

    	}


		private boolean sendPointerEvent(OutputEvent.PointerEvent pe) {

			try {
				if (rfb.inNormalProtocol) {
					bitmapData.invalidateMousePosition();

					try {
						rfb.writePointerEvent(pe.x,pe.y,pe.modifiers,pe.mask);
						return true;
					} catch (Exception e) {
						return false;
					}
				}
			}
			catch(NullPointerException e) {
			}
			return false;

		}


		private boolean sendKeyEvent(OutputEvent.KeyboardEvent evt) {
			if (rfb != null && rfb.inNormalProtocol) {

			   try {
				   if(Utils.DEBUG()) Log.d(TAG, "sending key " + evt.keyCode + (evt.down?" down":" up"));
				   rfb.writeKeyEvent(evt.keyCode, evt.metaState, evt.down);
				   return true;
				} catch (Exception e) {
					return false;
				}
			}
			return false;
		}


		private boolean sendCutText(String text) {
			if (rfb != null && rfb.inNormalProtocol) {

				try {
					if(Utils.DEBUG()) Log.d(TAG, "sending cuttext " + text);
					rfb.writeClientCutText(text);
					return true;
				} catch (Exception e) {
					return false;
				}
			}
			return false;
		}


    }




	public VNCConn() {
		handleRREPaint = new Paint();
		handleRREPaint.setStyle(Style.FILL);

		if(Utils.DEBUG()) Log.d(TAG, this + " constructed!");
	}

	protected void finalize() {
		if(Utils.DEBUG()) Log.d(TAG, this + " finalized!");
	}


	/*
		    to make a connection, call
		    Setup(), then
		    Listen() (optional), then
		    Init(), then
		    Shutdown, then
		    Cleanup()

		    NB: If Init() fails, you have to call Setup() again!

		    The semantic counterparts are:
		       Setup() <-> Cleanup()
		       Init()  <-> Shutdown()
	 */

	boolean Setup() {

		return true;
	}


	void Cleanup() {

	}



	/**
	 * Create a view showing a VNC connection
	 * @param bean Connection settings
	 * @param setModes Callback to run on UI thread after connection is set up
	 */
	public void init(ConnectionBean bean, final Runnable setModes) {

		Log.d(TAG, "initializing");

		connSettings = bean;
		try {
			this.pendingColorModel = COLORMODEL.valueOf(bean.getColorModel());
		}
		catch(IllegalArgumentException e) {
			this.pendingColorModel = COLORMODEL.C24bit;
		}

		// Startup the RFB thread with a nifty progess dialog
		final ProgressDialog pd = new ProgressDialog(canvas.getContext());
		pd.setCancelable(false); // on ICS, clicking somewhere cancels the dialog. not what we want...
	    pd.setTitle("Connecting...");
	    pd.setMessage("Establishing handshake.\nPlease wait...");
	    pd.setButton(DialogInterface.BUTTON_NEGATIVE, canvas.getContext().getString(android.R.string.cancel), new DialogInterface.OnClickListener()
	    {
	        public void onClick(DialogInterface dialog, int which)
	        {
	        	canvas.activity.finish();
	        }
	    });
	    pd.show();


		inputThread = new VncInputThread(pd, setModes);
		inputThread.start();
	}



	public void shutdown() {

		Log.d(TAG, "shutting down");

		maintainConnection = false;

		try {
			bitmapData.dispose();
			rfb.close(); // close immediatly

			// the input thread stops by itself, but the putput thread not
			outputThread.interrupt();
		}
		catch(Exception e) {
		}

		bitmapData = null;
		canvas = null;
		connSettings = null;

		System.gc();
	}

	/**
	 * Set/unset the canvas for this connection.
	 * Unset canvas when keeping the VNCConn across application restarts to avoid memleaks.
	 * @param c
	 */
	public void setCanvas(VncCanvas c) {
		canvas = c;
	}


	public boolean sendCutText(String text) {

		if(rfb != null && rfb.inNormalProtocol) { // only queue if already connected
			OutputEvent e = new OutputEvent(text);
			outputEventQueue.add(e);
			synchronized (outputEventQueue) {
				outputEventQueue.notify();
			}

			return true;
		}
		else
			return false;
	}


	public boolean sendPointerEvent(int x, int y, int modifiers, int pointerMask) {

		// trim coodinates
		if (x<0) x=0;
		else if (x>=rfb.framebufferWidth) x=rfb.framebufferWidth-1;
		if (y<0) y=0;
		else if (y>=rfb.framebufferHeight) y=rfb.framebufferHeight-1;

		if(rfb != null && rfb.inNormalProtocol) { // only queue if already connected
			OutputEvent e = new OutputEvent(x, y, modifiers, pointerMask);
			outputEventQueue.add(e);
			synchronized (outputEventQueue) {
				outputEventQueue.notify();
			}

			canvas.mouseX = x;
			canvas.mouseY = y;
			canvas.panToMouse();

			return true;
		}
		else
			return false;
	}

	/**
	 * queue key event for sending
	 * @param keyCode
	 * @param evt
	 * @param sendDirectly send key event directly without doing local->rfbKeySym translation
	 * @return
	 */
	public boolean sendKeyEvent(int keyCode, KeyEvent evt, boolean sendDirectly) {

		if(rfb != null && rfb.inNormalProtocol) { // only queue if already connected

			if(Utils.DEBUG()) Log.d(TAG, "queueing key evt " + evt.toString() + " code 0x" + Integer.toHexString(keyCode));

			// check if special char
			if(evt.getAction() == KeyEvent.ACTION_MULTIPLE && evt.getKeyCode() == KeyEvent.KEYCODE_UNKNOWN) {
				OutputEvent down = new OutputEvent(
						evt.getCharacters().codePointAt(0),
						evt.getMetaState(),
						true);
				outputEventQueue.add(down);

				OutputEvent up = new OutputEvent(
						evt.getCharacters().codePointAt(0),
						evt.getMetaState(),
						false);
				outputEventQueue.add(up);

				synchronized (outputEventQueue) {
					outputEventQueue.notify();
				}

			}
			// 'normal' key, i.e. either up or down
			else  {

				int metaState = evt.getMetaState();

				// only do translation for events that were *not* synthesized,
				// i.e. coming from a metakeybean which already is translated
				if(!sendDirectly) {
					switch(keyCode) {
					case KeyEvent.KEYCODE_BACK :        keyCode = 0xff1b; break;
					case KeyEvent.KEYCODE_DPAD_LEFT:    keyCode = 0xff51; break;
					case KeyEvent.KEYCODE_DPAD_UP:      keyCode = 0xff52; break;
					case KeyEvent.KEYCODE_DPAD_RIGHT:   keyCode = 0xff53; break;
					case KeyEvent.KEYCODE_DPAD_DOWN:    keyCode = 0xff54; break;
					case KeyEvent.KEYCODE_DEL: 		    keyCode = 0xff08; break;
					case KeyEvent.KEYCODE_ENTER:        keyCode = 0xff0d; break;
					case KeyEvent.KEYCODE_DPAD_CENTER:  keyCode = 0xff0d; break;
					case KeyEvent.KEYCODE_TAB:          keyCode = 0xff09; break;
					case KeyEvent.KEYCODE_ESCAPE:	    keyCode = 0xff1b; break;
					case KeyEvent.KEYCODE_ALT_LEFT:     keyCode = 0xffe9; break;
					case KeyEvent.KEYCODE_ALT_RIGHT:    keyCode = 0xffea; break;
					case KeyEvent.KEYCODE_F1: 			keyCode = 0xffbe; break;
					case KeyEvent.KEYCODE_F2:		    keyCode = 0xffbf; break;
					case KeyEvent.KEYCODE_F3: 			keyCode = 0xffc0; break;
					case KeyEvent.KEYCODE_F4: 			keyCode = 0xffc1; break;
					case KeyEvent.KEYCODE_F5: 			keyCode = 0xffc2; break;
					case KeyEvent.KEYCODE_F6: 			keyCode = 0xffc3; break;
					case KeyEvent.KEYCODE_F7: 			keyCode = 0xffc4; break;
					case KeyEvent.KEYCODE_F8: 			keyCode = 0xffc5; break;
					case KeyEvent.KEYCODE_F9: 			keyCode = 0xffc6; break;
					case KeyEvent.KEYCODE_F10: 			keyCode = 0xffc7; break;
					case KeyEvent.KEYCODE_F11: 			keyCode = 0xffc8; break;
					case KeyEvent.KEYCODE_F12: 			keyCode = 0xffc9; break;
					case KeyEvent.KEYCODE_INSERT:	    keyCode = 0xff63; break;
					case KeyEvent.KEYCODE_FORWARD_DEL:	keyCode = 0xffff; break;
					case KeyEvent.KEYCODE_MOVE_HOME:    keyCode = 0xff50; break;
					case KeyEvent.KEYCODE_MOVE_END:     keyCode = 0xff57; break;
					case KeyEvent.KEYCODE_PAGE_UP:      keyCode = 0xff55; break;
					case KeyEvent.KEYCODE_PAGE_DOWN:    keyCode = 0xff56; break;
					case KeyEvent.KEYCODE_CTRL_LEFT:    keyCode = 0xffe3; break;
					case KeyEvent.KEYCODE_CTRL_RIGHT:   keyCode = 0xffe4; break;
					default:
						// do keycode -> UTF-8 keysym conversion
						KeyEvent tmp = new KeyEvent(
								0,
								0,
								0,
								keyCode,
								0,
								metaState);

						keyCode = tmp.getUnicodeChar();

						// Ctrl-C for example needs this...
						if (keyCode == 0) {
							metaState &= ~KeyEvent.META_CTRL_MASK;
							metaState &= ~KeyEvent.META_ALT_MASK;
							keyCode = tmp.getUnicodeChar(metaState);
						}

						metaState = 0;
						break;

					}
				}

				OutputEvent e = new OutputEvent(
						keyCode,
						metaState,
						evt.getAction() == KeyEvent.ACTION_DOWN);
				outputEventQueue.add(e);
				synchronized (outputEventQueue) {
					outputEventQueue.notify();
				}

			}

			return true;
		}
		else
			return false;
	}



	public boolean toggleFramebufferUpdates()
	{
		framebufferUpdatesEnabled = !framebufferUpdatesEnabled;
		if(framebufferUpdatesEnabled)
		{
			try {
				// clear old framebuffer
				bitmapData.clear();
				// request full non-incremental update to get going again
				OutputEvent e = new OutputEvent(false);
				outputEventQueue.add(e);
				synchronized (outputEventQueue) {
					outputEventQueue.notify();
				}
			} catch (Exception e) {
				e.printStackTrace();
			}
		}

		return framebufferUpdatesEnabled;
	}



	public void setColorModel(COLORMODEL cm) {
		// Only update if color model changes
		if (colorModel == null || !colorModel.equals(cm))
			pendingColorModel = cm;
	}



	public final COLORMODEL getColorModel() {
		return colorModel;
	}


	public String getEncoding() {
		switch (preferredEncoding) {
		case RfbProto.EncodingRaw:
			return "RAW";
		case RfbProto.EncodingTight:
			return "TIGHT";
		case RfbProto.EncodingCoRRE:
			return "CoRRE";
		case RfbProto.EncodingHextile:
			return "HEXTILE";
		case RfbProto.EncodingRRE:
			return "RRE";
		case RfbProto.EncodingZlib:
			return "ZLIB";
		case RfbProto.EncodingZRLE:
			return "ZRLE";
		}
		return "";
	}


	public final String getDesktopName() {
		return rfb.desktopName;
	}

	public final int getFramebufferWidth() {
		try {
			return bitmapData.framebufferwidth;
		}
		catch(NullPointerException e) {
			return 0;
		}
	}

	public final int getFramebufferHeight() {
		try {
			return bitmapData.framebufferheight;
		}
		catch(NullPointerException e) {
			return 0;
		}
	}

	public String getCutText() {
		return serverCutText;
	}

	public final AbstractBitmapData getFramebuffer() {
		return bitmapData;
	}

	public void lockFramebuffer() {
		bitmapDataPixelsLock.lock();
	}

	public void unlockFramebuffer() {
		bitmapDataPixelsLock.unlock();
	}


	public final ConnectionBean getConnSettings() {
		return connSettings;
	}











	/**
	 * Additional Encodings
	 *
	 */

	private void setEncodings(boolean autoSelectOnly) {
		if (rfb == null || !rfb.inNormalProtocol)
			return;

		if (preferredEncoding == -1) {
			// Preferred format is ZRLE
			preferredEncoding = RfbProto.EncodingZRLE;
		} else {
			// Auto encoder selection is not enabled.
			if (autoSelectOnly)
				return;
		}

		int[] encodings = new int[20];
		int nEncodings = 0;

		encodings[nEncodings++] = preferredEncoding;
		if (useCopyRect)
			encodings[nEncodings++] = RfbProto.EncodingCopyRect;
		// if (preferredEncoding != RfbProto.EncodingTight)
		// encodings[nEncodings++] = RfbProto.EncodingTight;
		if (preferredEncoding != RfbProto.EncodingZRLE)
			encodings[nEncodings++] = RfbProto.EncodingZRLE;
		if (preferredEncoding != RfbProto.EncodingHextile)
			encodings[nEncodings++] = RfbProto.EncodingHextile;
		if (preferredEncoding != RfbProto.EncodingZlib)
			encodings[nEncodings++] = RfbProto.EncodingZlib;
		if (preferredEncoding != RfbProto.EncodingCoRRE)
			encodings[nEncodings++] = RfbProto.EncodingCoRRE;
		if (preferredEncoding != RfbProto.EncodingRRE)
			encodings[nEncodings++] = RfbProto.EncodingRRE;

		if (compressLevel >= 0 && compressLevel <= 9)
			encodings[nEncodings++] = RfbProto.EncodingCompressLevel0 + compressLevel;
		if (jpegQuality >= 0 && jpegQuality <= 9)
			encodings[nEncodings++] = RfbProto.EncodingQualityLevel0 + jpegQuality;

		encodings[nEncodings++] = RfbProto.EncodingLastRect;
		encodings[nEncodings++] = RfbProto.EncodingNewFBSize;
		encodings[nEncodings++] = RfbProto.EncodingPointerPos;


		boolean encodingsWereChanged = false;
		if (nEncodings != nEncodingsSaved) {
			encodingsWereChanged = true;
		} else {
			for (int i = 0; i < nEncodings; i++) {
				if (encodings[i] != encodingsSaved[i]) {
					encodingsWereChanged = true;
					break;
				}
			}
		}

		if (encodingsWereChanged) {
			try {
				rfb.writeSetEncodings(encodings, nEncodings);
			} catch (Exception e) {
				e.printStackTrace();
			}
			encodingsSaved = encodings;
			nEncodingsSaved = nEncodings;
		}
	}




	//
	// Handle a CopyRect rectangle.
	//

	private final Paint handleCopyRectPaint = new Paint();
	private void handleCopyRect(int x, int y, int w, int h) throws IOException {

		/**
		 * This does not work properly yet.
		 */

		rfb.readCopyRect();
		if ( ! bitmapData.validDraw(x, y, w, h))
			return;
		// Source Coordinates
		int leftSrc = rfb.copyRectSrcX;
		int topSrc = rfb.copyRectSrcY;
		int rightSrc = topSrc + w;
		int bottomSrc = topSrc + h;

		// Change
		int dx = x - rfb.copyRectSrcX;
		int dy = y - rfb.copyRectSrcY;

		// Destination Coordinates
		int leftDest = leftSrc + dx;
		int topDest = topSrc + dy;
		int rightDest = rightSrc + dx;
		int bottomDest = bottomSrc + dy;

		bitmapData.copyRect(new Rect(leftSrc, topSrc, rightSrc, bottomSrc), new Rect(leftDest, topDest, rightDest, bottomDest), handleCopyRectPaint);

		canvas.reDraw();
	}
	private byte[] bg_buf = new byte[4];
	private byte[] rre_buf = new byte[128];
	//
	// Handle an RRE-encoded rectangle.
	//
	private void handleRRERect(int x, int y, int w, int h) throws IOException {
		boolean valid=bitmapData.validDraw(x, y, w, h);
		int nSubrects = rfb.is.readInt();

		rfb.readFully(bg_buf, 0, bytesPerPixel);
		int pixel;
		if (bytesPerPixel == 1) {
			pixel = colorPalette[0xFF & bg_buf[0]];
		} else {
			pixel = Color.rgb(bg_buf[2] & 0xFF, bg_buf[1] & 0xFF, bg_buf[0] & 0xFF);
		}
		handleRREPaint.setColor(pixel);
		if ( valid)
			bitmapData.drawRect(x, y, w, h, handleRREPaint);

		int len = nSubrects * (bytesPerPixel + 8);
		if (len > rre_buf.length)
			rre_buf = new byte[len];

		rfb.readFully(rre_buf, 0, len);
		if ( ! valid)
			return;

		int sx, sy, sw, sh;

		int i = 0;
		for (int j = 0; j < nSubrects; j++) {
			if (bytesPerPixel == 1) {
				pixel = colorPalette[0xFF & rre_buf[i++]];
			} else {
				pixel = Color.rgb(rre_buf[i + 2] & 0xFF, rre_buf[i + 1] & 0xFF, rre_buf[i] & 0xFF);
				i += 4;
			}
			sx = x + ((rre_buf[i] & 0xff) << 8) + (rre_buf[i+1] & 0xff); i+=2;
			sy = y + ((rre_buf[i] & 0xff) << 8) + (rre_buf[i+1] & 0xff); i+=2;
			sw = ((rre_buf[i] & 0xff) << 8) + (rre_buf[i+1] & 0xff); i+=2;
			sh = ((rre_buf[i] & 0xff) << 8) + (rre_buf[i+1] & 0xff); i+=2;

			handleRREPaint.setColor(pixel);
			bitmapData.drawRect(sx, sy, sw, sh, handleRREPaint);
		}

		canvas.reDraw();
	}

	//
	// Handle a CoRRE-encoded rectangle.
	//

	private void handleCoRRERect(int x, int y, int w, int h) throws IOException {
		boolean valid=bitmapData.validDraw(x, y, w, h);
		int nSubrects = rfb.is.readInt();

		rfb.readFully(bg_buf, 0, bytesPerPixel);
		int pixel;
		if (bytesPerPixel == 1) {
			pixel = colorPalette[0xFF & bg_buf[0]];
		} else {
			pixel = Color.rgb(bg_buf[2] & 0xFF, bg_buf[1] & 0xFF, bg_buf[0] & 0xFF);
		}
		handleRREPaint.setColor(pixel);
		if ( valid)
			bitmapData.drawRect(x, y, w, h, handleRREPaint);

		int len = nSubrects * (bytesPerPixel + 8);
		if (len > rre_buf.length)
			rre_buf = new byte[len];

		rfb.readFully(rre_buf, 0, len);
		if ( ! valid)
			return;

		int sx, sy, sw, sh;
		int i = 0;

		for (int j = 0; j < nSubrects; j++) {
			if (bytesPerPixel == 1) {
				pixel = colorPalette[0xFF & rre_buf[i++]];
			} else {
				pixel = Color.rgb(rre_buf[i + 2] & 0xFF, rre_buf[i + 1] & 0xFF, rre_buf[i] & 0xFF);
				i += 4;
			}
			sx = x + (rre_buf[i++] & 0xFF);
			sy = y + (rre_buf[i++] & 0xFF);
			sw = rre_buf[i++] & 0xFF;
			sh = rre_buf[i++] & 0xFF;

			handleRREPaint.setColor(pixel);
			bitmapData.drawRect(sx, sy, sw, sh, handleRREPaint);
		}

		canvas.reDraw();
	}

	//
	// Handle a Hextile-encoded rectangle.
	//

	// These colors should be kept between handleHextileSubrect() calls.
	private int hextile_bg, hextile_fg;

	private void handleHextileRect(int x, int y, int w, int h) throws IOException {

		hextile_bg = Color.BLACK;
		hextile_fg = Color.BLACK;

		for (int ty = y; ty < y + h; ty += 16) {
			int th = 16;
			if (y + h - ty < 16)
				th = y + h - ty;

			for (int tx = x; tx < x + w; tx += 16) {
				int tw = 16;
				if (x + w - tx < 16)
					tw = x + w - tx;

				handleHextileSubrect(tx, ty, tw, th);
			}

			// Finished with a row of tiles, now let's show it.
			canvas.reDraw();
		}
	}

	//
	// Handle one tile in the Hextile-encoded data.
	//

	private Paint handleHextileSubrectPaint = new Paint();
	private byte[] backgroundColorBuffer = new byte[4];
	private void handleHextileSubrect(int tx, int ty, int tw, int th) throws IOException {

		int subencoding = rfb.is.readUnsignedByte();

		// Is it a raw-encoded sub-rectangle?
		if ((subencoding & RfbProto.HextileRaw) != 0) {
			handleRawRect(tx, ty, tw, th, false);
			return;
		}

		boolean valid=bitmapData.validDraw(tx, ty, tw, th);
		// Read and draw the background if specified.
		if (bytesPerPixel > backgroundColorBuffer.length) {
			throw new RuntimeException("impossible colordepth");
		}
		if ((subencoding & RfbProto.HextileBackgroundSpecified) != 0) {
			rfb.readFully(backgroundColorBuffer, 0, bytesPerPixel);
			if (bytesPerPixel == 1) {
				hextile_bg = colorPalette[0xFF & backgroundColorBuffer[0]];
			} else {
				hextile_bg = Color.rgb(backgroundColorBuffer[2] & 0xFF, backgroundColorBuffer[1] & 0xFF, backgroundColorBuffer[0] & 0xFF);
			}
		}
		handleHextileSubrectPaint.setColor(hextile_bg);
		handleHextileSubrectPaint.setStyle(Paint.Style.FILL);
		if ( valid )
			bitmapData.drawRect(tx, ty, tw, th, handleHextileSubrectPaint);

		// Read the foreground color if specified.
		if ((subencoding & RfbProto.HextileForegroundSpecified) != 0) {
			rfb.readFully(backgroundColorBuffer, 0, bytesPerPixel);
			if (bytesPerPixel == 1) {
				hextile_fg = colorPalette[0xFF & backgroundColorBuffer[0]];
			} else {
				hextile_fg = Color.rgb(backgroundColorBuffer[2] & 0xFF, backgroundColorBuffer[1] & 0xFF, backgroundColorBuffer[0] & 0xFF);
			}
		}

		// Done with this tile if there is no sub-rectangles.
		if ((subencoding & RfbProto.HextileAnySubrects) == 0)
			return;

		int nSubrects = rfb.is.readUnsignedByte();
		int bufsize = nSubrects * 2;
		if ((subencoding & RfbProto.HextileSubrectsColoured) != 0) {
			bufsize += nSubrects * bytesPerPixel;
		}
		if (rre_buf.length < bufsize)
			rre_buf = new byte[bufsize];
		rfb.readFully(rre_buf, 0, bufsize);

		int b1, b2, sx, sy, sw, sh;
		int i = 0;
		if ((subencoding & RfbProto.HextileSubrectsColoured) == 0) {

			// Sub-rectangles are all of the same color.
			handleHextileSubrectPaint.setColor(hextile_fg);
			for (int j = 0; j < nSubrects; j++) {
				b1 = rre_buf[i++] & 0xFF;
				b2 = rre_buf[i++] & 0xFF;
				sx = tx + (b1 >> 4);
				sy = ty + (b1 & 0xf);
				sw = (b2 >> 4) + 1;
				sh = (b2 & 0xf) + 1;
				if ( valid)
					bitmapData.drawRect(sx, sy, sw, sh, handleHextileSubrectPaint);
			}
		} else if (bytesPerPixel == 1) {

			// BGR233 (8-bit color) version for colored sub-rectangles.
			for (int j = 0; j < nSubrects; j++) {
				hextile_fg = colorPalette[0xFF & rre_buf[i++]];
				b1 = rre_buf[i++] & 0xFF;
				b2 = rre_buf[i++] & 0xFF;
				sx = tx + (b1 >> 4);
				sy = ty + (b1 & 0xf);
				sw = (b2 >> 4) + 1;
				sh = (b2 & 0xf) + 1;
				handleHextileSubrectPaint.setColor(hextile_fg);
				if ( valid)
					bitmapData.drawRect(sx, sy, sw, sh, handleHextileSubrectPaint);
			}

		} else {

			// Full-color (24-bit) version for colored sub-rectangles.
			for (int j = 0; j < nSubrects; j++) {
				hextile_fg = Color.rgb(rre_buf[i + 2] & 0xFF, rre_buf[i + 1] & 0xFF, rre_buf[i] & 0xFF);
				i += 4;
				b1 = rre_buf[i++] & 0xFF;
				b2 = rre_buf[i++] & 0xFF;
				sx = tx + (b1 >> 4);
				sy = ty + (b1 & 0xf);
				sw = (b2 >> 4) + 1;
				sh = (b2 & 0xf) + 1;
				handleHextileSubrectPaint.setColor(hextile_fg);
				if ( valid )
					bitmapData.drawRect(sx, sy, sw, sh, handleHextileSubrectPaint);
			}

		}
	}

	//
	// Handle a ZRLE-encoded rectangle.
	//

	private Paint handleZRLERectPaint = new Paint();
	private int[] handleZRLERectPalette = new int[128];
	private void handleZRLERect(int x, int y, int w, int h) throws Exception {

		if (zrleInStream == null)
			zrleInStream = new ZlibInStream();

		int nBytes = rfb.is.readInt();
		if (nBytes > 64 * 1024 * 1024)
			throw new Exception("ZRLE decoder: illegal compressed data size");

		if (zrleBuf == null || zrleBuf.length < nBytes) {
			zrleBuf = new byte[nBytes+4096];
		}

		rfb.readFully(zrleBuf, 0, nBytes);

		zrleInStream.setUnderlying(new MemInStream(zrleBuf, 0, nBytes), nBytes);

		boolean valid=bitmapData.validDraw(x, y, w, h);

		for (int ty = y; ty < y + h; ty += 64) {

			int th = Math.min(y + h - ty, 64);

			for (int tx = x; tx < x + w; tx += 64) {

				int tw = Math.min(x + w - tx, 64);

				int mode = zrleInStream.readU8();
				boolean rle = (mode & 128) != 0;
				int palSize = mode & 127;

				readZrlePalette(handleZRLERectPalette, palSize);

				if (palSize == 1) {
					int pix = handleZRLERectPalette[0];
					int c = (bytesPerPixel == 1) ? colorPalette[0xFF & pix] : (0xFF000000 | pix);
					handleZRLERectPaint.setColor(c);
					handleZRLERectPaint.setStyle(Paint.Style.FILL);
					if ( valid)
						bitmapData.drawRect(tx, ty, tw, th, handleZRLERectPaint);
					continue;
				}

				if (!rle) {
					if (palSize == 0) {
						readZrleRawPixels(tw, th);
					} else {
						readZrlePackedPixels(tw, th, handleZRLERectPalette, palSize);
					}
				} else {
					if (palSize == 0) {
						readZrlePlainRLEPixels(tw, th);
					} else {
						readZrlePackedRLEPixels(tw, th, handleZRLERectPalette);
					}
				}
				if ( valid )
					handleUpdatedZrleTile(tx, ty, tw, th);
			}
		}

		zrleInStream.reset();

		canvas.reDraw();
	}

	//
	// Handle a Zlib-encoded rectangle.
	//

	private byte[] handleZlibRectBuffer = new byte[128];
	private void handleZlibRect(int x, int y, int w, int h) throws Exception {
		boolean valid = bitmapData.validDraw(x, y, w, h);
		int nBytes = rfb.is.readInt();

		if (zlibBuf == null || zlibBuf.length < nBytes) {
			zlibBuf = new byte[nBytes*2];
		}

		rfb.readFully(zlibBuf, 0, nBytes);

		if (zlibInflater == null) {
			zlibInflater = new Inflater();
		}
		zlibInflater.setInput(zlibBuf, 0, nBytes);

		bitmapDataPixelsLock.lock();

		int[] pixels=bitmapData.bitmapPixels;

		if (bytesPerPixel == 1) {
			// 1 byte per pixel. Use palette lookup table.
			if (w > handleZlibRectBuffer.length) {
				handleZlibRectBuffer = new byte[w];
			}
			int i, offset;
			for (int dy = y; dy < y + h; dy++) {
				zlibInflater.inflate(handleZlibRectBuffer,  0, w);
				if ( ! valid)
					continue;
				offset = bitmapData.offset(x, dy);
				for (i = 0; i < w; i++) {
					pixels[offset + i] = colorPalette[0xFF & handleZlibRectBuffer[i]];
				}
			}
		} else {
			// 24-bit color (ARGB) 4 bytes per pixel.
			final int l = w*4;
			if (l > handleZlibRectBuffer.length) {
				handleZlibRectBuffer = new byte[l];
			}
			int i, offset;
			for (int dy = y; dy < y + h; dy++) {
				zlibInflater.inflate(handleZlibRectBuffer, 0, l);
				if ( ! valid)
					continue;
				offset = bitmapData.offset(x, dy);
				for (i = 0; i < w; i++) {
					final int idx = i*4;
					pixels[offset + i] = (handleZlibRectBuffer[idx + 2] & 0xFF) << 16 | (handleZlibRectBuffer[idx + 1] & 0xFF) << 8 | (handleZlibRectBuffer[idx] & 0xFF);
				}
			}
		}

		bitmapDataPixelsLock.unlock();

		if ( ! valid)
			return;
		bitmapData.updateBitmap(x, y, w, h);

		canvas.reDraw();
	}

	private int readPixel(InStream is) throws Exception {
		int pix;
		if (bytesPerPixel == 1) {
			pix = is.readU8();
		} else {
			int p1 = is.readU8();
			int p2 = is.readU8();
			int p3 = is.readU8();
			pix = (p3 & 0xFF) << 16 | (p2 & 0xFF) << 8 | (p1 & 0xFF);
		}
		return pix;
	}

	private byte[] readPixelsBuffer = new byte[128];
	private void readPixels(InStream is, int[] dst, int count) throws Exception {
		if (bytesPerPixel == 1) {
			if (count > readPixelsBuffer.length) {
				readPixelsBuffer = new byte[count];
			}
			is.readBytes(readPixelsBuffer, 0, count);
			for (int i = 0; i < count; i++) {
				dst[i] = (int) readPixelsBuffer[i] & 0xFF;
			}
		} else {
			final int l = count * 3;
			if (l > readPixelsBuffer.length) {
				readPixelsBuffer = new byte[l];
			}
			is.readBytes(readPixelsBuffer, 0, l);
			for (int i = 0; i < count; i++) {
				final int idx = i*3;
				dst[i] = ((readPixelsBuffer[idx + 2] & 0xFF) << 16 | (readPixelsBuffer[idx + 1] & 0xFF) << 8 | (readPixelsBuffer[idx] & 0xFF));
			}
		}
	}

	private void readZrlePalette(int[] palette, int palSize) throws Exception {
		readPixels(zrleInStream, palette, palSize);
	}

	private void readZrleRawPixels(int tw, int th) throws Exception {
		int len = tw * th;
		if (zrleTilePixels == null || len > zrleTilePixels.length)
			zrleTilePixels = new int[len];
		readPixels(zrleInStream, zrleTilePixels, tw * th); // /
	}

	private void readZrlePackedPixels(int tw, int th, int[] palette, int palSize) throws Exception {

		int bppp = ((palSize > 16) ? 8 : ((palSize > 4) ? 4 : ((palSize > 2) ? 2 : 1)));
		int ptr = 0;
		int len = tw * th;
		if (zrleTilePixels == null || len > zrleTilePixels.length)
			zrleTilePixels = new int[len];

		for (int i = 0; i < th; i++) {
			int eol = ptr + tw;
			int b = 0;
			int nbits = 0;

			while (ptr < eol) {
				if (nbits == 0) {
					b = zrleInStream.readU8();
					nbits = 8;
				}
				nbits -= bppp;
				int index = (b >> nbits) & ((1 << bppp) - 1) & 127;
				if (bytesPerPixel == 1) {
					if (index >= colorPalette.length)
						Log.e(TAG, "zrlePlainRLEPixels palette lookup out of bounds " + index + " (0x" + Integer.toHexString(index) + ")");
					zrleTilePixels[ptr++] = colorPalette[0xFF & palette[index]];
				} else {
					zrleTilePixels[ptr++] = palette[index];
				}
			}
		}
	}

	private void readZrlePlainRLEPixels(int tw, int th) throws Exception {
		int ptr = 0;
		int end = ptr + tw * th;
		if (zrleTilePixels == null || end > zrleTilePixels.length)
			zrleTilePixels = new int[end];
		while (ptr < end) {
			int pix = readPixel(zrleInStream);
			int len = 1;
			int b;
			do {
				b = zrleInStream.readU8();
				len += b;
			} while (b == 255);

			if (!(len <= end - ptr))
				throw new Exception("ZRLE decoder: assertion failed" + " (len <= end-ptr)");

			if (bytesPerPixel == 1) {
				while (len-- > 0)
					zrleTilePixels[ptr++] = colorPalette[0xFF & pix];
			} else {
				while (len-- > 0)
					zrleTilePixels[ptr++] = pix;
			}
		}
	}

	private void readZrlePackedRLEPixels(int tw, int th, int[] palette) throws Exception {

		int ptr = 0;
		int end = ptr + tw * th;
		if (zrleTilePixels == null || end > zrleTilePixels.length)
			zrleTilePixels = new int[end];
		while (ptr < end) {
			int index = zrleInStream.readU8();
			int len = 1;
			if ((index & 128) != 0) {
				int b;
				do {
					b = zrleInStream.readU8();
					len += b;
				} while (b == 255);

				if (!(len <= end - ptr))
					throw new Exception("ZRLE decoder: assertion failed" + " (len <= end - ptr)");
			}

			index &= 127;
			int pix = palette[index];

			if (bytesPerPixel == 1) {
				while (len-- > 0)
					zrleTilePixels[ptr++] = colorPalette[0xFF & pix];
			} else {
				while (len-- > 0)
					zrleTilePixels[ptr++] = pix;
			}
		}
	}

	//
	// Copy pixels from zrleTilePixels8 or zrleTilePixels24, then update.
	//

	private void handleUpdatedZrleTile(int x, int y, int w, int h) {
		int offsetSrc = 0;

		bitmapDataPixelsLock.lock();

		int[] destPixels=bitmapData.bitmapPixels;
		for (int j = 0; j < h; j++) {
			System.arraycopy(zrleTilePixels, offsetSrc, destPixels, bitmapData.offset(x, y + j), w);
			offsetSrc += w;
		}

		bitmapDataPixelsLock.unlock();

		bitmapData.updateBitmap(x, y, w, h);
	}



	private void handleRawRect(int x, int y, int w, int h) throws IOException {
		handleRawRect(x, y, w, h, true);
	}

	private byte[] handleRawRectBuffer = new byte[128];
	private void handleRawRect(int x, int y, int w, int h, boolean paint) throws IOException {
		boolean valid=bitmapData.validDraw(x, y, w, h);

		bitmapDataPixelsLock.lock();

		int[] pixels=bitmapData.bitmapPixels;
		if (bytesPerPixel == 1) {
			// 1 byte per pixel. Use palette lookup table.
			if (w > handleRawRectBuffer.length) {
				handleRawRectBuffer = new byte[w];
			}
			int i, offset;
			for (int dy = y; dy < y + h; dy++) {
				rfb.readFully(handleRawRectBuffer, 0, w);
				if ( ! valid)
					continue;
				offset = bitmapData.offset(x, dy);
				for (i = 0; i < w; i++) {
					pixels[offset + i] = colorPalette[0xFF & handleRawRectBuffer[i]];
				}
			}
		} else {
			// 4 bytes per pixel (argb) 24-bit color

			final int l = w * 4;
			if (l>handleRawRectBuffer.length) {
				handleRawRectBuffer = new byte[l];
			}
			int i, offset;
			for (int dy = y; dy < y + h; dy++) {
				rfb.readFully(handleRawRectBuffer, 0, l);
				if ( ! valid)
					continue;
				offset = bitmapData.offset(x, dy);
				for (i = 0; i < w; i++) {
					final int idx = i*4;
					pixels[offset + i] = // 0xFF << 24 |
						(handleRawRectBuffer[idx + 2] & 0xff) << 16 | (handleRawRectBuffer[idx + 1] & 0xff) << 8 | (handleRawRectBuffer[idx] & 0xff);
				}
			}
		}

		bitmapDataPixelsLock.unlock();

		if ( ! valid)
			return;

		bitmapData.updateBitmap( x, y, w, h);

		if (paint)
			canvas.reDraw();
	}



}

