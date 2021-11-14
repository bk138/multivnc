/**
 *
 * This represents an *active* VNC connection (as opposed to ConnectionBean, which is more like a bookmark.).
 *
 * Copyright (C) 2012 Christian Beier <dontmind@freeshell.org>
 */


package com.coboltforge.dontmind.multivnc;

import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.util.Log;
import android.view.KeyEvent;
import androidx.annotation.Keep;


public class VNCConn {
	static {
		System.loadLibrary("vncconn");
	}

	private final static String TAG = "VNCConn";

	private VncCanvas canvas;

	private ServerToClientThread inputThread;
	private ClientToServerThread outputThread;

	// the native rfbClient
	@Keep
	long rfbClient;
	private ConnectionBean connSettings;
	private COLORMODEL pendingColorModel = COLORMODEL.C24bit;

	// Runtime control flags
	private boolean maintainConnection = true;
	private boolean framebufferUpdatesEnabled = true;

	private Lock bitmapDataPixelsLock = new ReentrantLock();

	// message queue for communicating with the output worker thread
	private ConcurrentLinkedQueue<OutputEvent> outputEventQueue = new ConcurrentLinkedQueue<VNCConn.OutputEvent>();

	private COLORMODEL colorModel;

	private String serverCutText;

	// VNC Encoding parameters
	private int preferredEncoding = -1;

	// Useful shortcuts for modifier masks.
    public final static int CTRL_MASK  = KeyEvent.META_SYM_ON;
    public final static int SHIFT_MASK = KeyEvent.META_SHIFT_ON;
    public final static int META_MASK  = 0;
    public final static int ALT_MASK   = KeyEvent.META_ALT_ON;
	public final static int SUPER_MASK = KeyEvent.META_FUNCTION_ON; // mhm rather sym_on?

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

		public OutputEvent(int x, int y, int w, int h, boolean incremental) {
			fur = new FramebufferUpdateRequest();
			fur.x = x;
			fur.y = y;
			fur.w = w;
			fur.h = h;
			fur.incremental = incremental;
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

		private class FramebufferUpdateRequest {
    		int x,y,w,h;
			boolean incremental;
		}

    	private class ClientCutText {
    		String text;
    	}

    	public FullFramebufferUpdateRequest ffur;
		public FramebufferUpdateRequest fur;
		public PointerEvent pointer;
    	public KeyboardEvent key;
    	public ClientCutText cuttext;
    }


    private class ServerToClientThread extends Thread {

    	private ProgressDialog pd;
    	private Runnable setModes;


    	public ServerToClientThread(ProgressDialog pd, Runnable setModes) {
    		this.pd = pd;
    		this.setModes = setModes;
    	}


		public void run() {

			if(Utils.DEBUG()) Log.d(TAG, "ServerToClientThread started!");

			Context appContext = canvas.getContext().getApplicationContext();

			try {

				/*
				 * if IPv6 address, add scope id
				 */
				try {
					InetAddress address = InetAddress.getByName(connSettings.address);

					Inet6Address in6 = Inet6Address.getByAddress(
							address.getHostName(),
							address.getAddress(),
							Utils.getActiveNetworkInterface(canvas.getContext()));

					connSettings.address = in6.getHostAddress();
					Log.i(TAG, "Using IPv6");

				} catch (UnknownHostException e) {
					Log.i(TAG, "Using IPv4: " + e.toString());
				} catch (NullPointerException ne) {
					Log.e(TAG, ne.toString());
				}


				int repeaterId = (connSettings.useRepeater && connSettings.repeaterId != null && connSettings.repeaterId.length() > 0) ? Integer.parseInt(connSettings.repeaterId) : -1;
				lockFramebuffer();
				if (!rfbInit(connSettings.address, connSettings.port, repeaterId, pendingColorModel.bpp())) {
					unlockFramebuffer();
					throw new Exception(); //TODO add some error reoprting here
				}
				colorModel = pendingColorModel;
				unlockFramebuffer();

				// register connection
				VNCConnService.register(appContext, VNCConn.this);

				canvas.handler.post(new Runnable() {
					public void run() {
						canvas.activity.setTitle(getDesktopName());
						pd.setMessage("Downloading first frame.\nPlease wait...");
					}
				});

				// start output thread here
				outputThread = new ClientToServerThread();
				outputThread.start();

				// actually set scale type with this, otherwise no scaling
				canvas.activity.setModes();
				// center pointer
				canvas.mouseX = getFramebufferWidth() / 2;
				canvas.mouseY = getFramebufferHeight() / 2;
				// main loop
				while (maintainConnection) {
					if (!rfbProcessServerMessage()) {
						throw new Exception();
					}
				}
			} catch (Throwable e) {
				if (maintainConnection) {
					Log.e(TAG, e.toString());
					e.printStackTrace();
					// Ensure we dismiss the progress dialog
					// before we fatal error finish
					canvas.handler.post(new Runnable() {
						public void run() {
							try {
								if (pd.isShowing())
									pd.dismiss();
							} catch (Exception e) {
								//unused
							}
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
						final String error_ = error + "<br>" + ((e.getLocalizedMessage() != null) ? e.getLocalizedMessage() : "");
						canvas.handler.post(new Runnable() {
							public void run() {
								try {
									Utils.showFatalErrorMessage(canvas.getContext(), error_);
								}
								catch(NullPointerException e) {
								}
							}
						});
					}
				}
			}

			// we might get here when maintainConnection is set to false or when an exception was thrown
			lockFramebuffer(); // make sure the native texture drawing is not accessing something invalid
			rfbShutdown();
			unlockFramebuffer();

			// deregister connection
			VNCConnService.deregister(appContext, VNCConn.this);

			if(Utils.DEBUG()) Log.d(TAG, "ServerToClientThread done!");
		}

    }




    private class ClientToServerThread extends Thread {

    	public void run() {

			if(Utils.DEBUG()) Log.d(TAG, "ClientToServerThread started!");

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
							// bitmapData.writeFullUpdateRequest(ev.ffur.incremental);
						} catch (Exception e) {
							e.printStackTrace();
						}
					if(ev.fur != null)
						try {
							// rfb.writeFramebufferUpdateRequest(ev.fur.x, ev.fur.y, ev.fur.w, ev.fur.h, ev.fur.incremental);
						} catch (Exception e) {
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

    		if(Utils.DEBUG()) Log.d(TAG, "ClientToServerThread done!");

    	}


		private boolean sendPointerEvent(OutputEvent.PointerEvent pe) {
			return rfbSendPointerEvent(pe.x, pe.y, pe.mask);
		}


		private boolean sendKeyEvent(OutputEvent.KeyboardEvent evt) {
			if (rfbClient != 0) {

			   try {

				   if((evt.metaState & VNCConn.SHIFT_MASK) != 0) {
					   if(Utils.DEBUG()) Log.d(TAG, "sending key Shift" + (evt.down?" down":" up"));
					   rfbSendKeyEvent(0xffe1, evt.down);
				   }
				   if((evt.metaState & VNCConn.CTRL_MASK) != 0) {
					   if(Utils.DEBUG()) Log.d(TAG, "sending key Ctrl" + (evt.down?" down":" up"));
					   rfbSendKeyEvent(0xffe3, evt.down);
				   }
				   if((evt.metaState & VNCConn.ALT_MASK) != 0) {
					   if(Utils.DEBUG()) Log.d(TAG, "sending key Alt" + (evt.down?" down":" up"));
					   rfbSendKeyEvent(0xffe9, evt.down);
				   }
				   if((evt.metaState & VNCConn.SUPER_MASK) != 0) {
					   if(Utils.DEBUG()) Log.d(TAG, "sending key Super" + (evt.down?" down":" up"));
					   rfbSendKeyEvent(0xffeb, evt.down);
				   }
				   if((evt.metaState & VNCConn.META_MASK) != 0) {
					   if(Utils.DEBUG()) Log.d(TAG, "sending key Meta" + (evt.down?" down":" up"));
					   rfbSendKeyEvent(0xffe7, evt.down);
				   }

				   if(Utils.DEBUG()) Log.d(TAG, "sending key " + evt.keyCode + (evt.down?" down":" up"));
				   rfbSendKeyEvent(evt.keyCode, evt.down);

				   return true;
				} catch (Exception e) {
					return false;
				}
			}
			return false;
		}


		private boolean sendCutText(String text) {
			if (rfbClient != 0) {
				if (Utils.DEBUG()) Log.d(TAG, "sending cuttext " + text);
				return rfbSendClientCutText(text);
			}
			return false;
		}


    }


	private native boolean rfbInit(String host, int port, int repeaterId, int bytesPerPixel);
	private native void rfbShutdown();
	private native boolean rfbProcessServerMessage();
	private native boolean rfbSetFramebufferUpdatesEnabled(boolean enable);
	private native String rfbGetDesktopName();
	private native int rfbGetFramebufferWidth();
	private native int rfbGetFramebufferHeight();
	private native boolean rfbSendKeyEvent(long keysym, boolean down);
	private native boolean rfbSendPointerEvent(int x, int y, int buttonMask);
	private native boolean rfbSendClientCutText(String text);


	public VNCConn() {
		if(Utils.DEBUG()) Log.d(TAG, this + " constructed!");
	}

	protected void finalize() {
		if(Utils.DEBUG()) Log.d(TAG, this + " finalized!");
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
			this.pendingColorModel = COLORMODEL.valueOf(bean.colorModel);
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
		canvas.activity.firstFrameWaitDialog = pd;

		inputThread = new ServerToClientThread(pd, setModes);
		inputThread.start();
	}


	public void shutdown() {

		Log.d(TAG, "shutting down");

		maintainConnection = false;

		try {
			// the input thread stops by itself, but the putput thread not
			outputThread.interrupt();
		}
		catch(Exception e) {
		}

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

		if(rfbClient != 0) { // only queue if already connected
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


		if(rfbClient != 0) { // only queue if already connected

			// trim coodinates
			if (x<0) x=0;
			else if (x>=getFramebufferWidth()) x=getFramebufferWidth()-1;
			if (y<0) y=0;
			else if (y>=getFramebufferHeight()) y=getFramebufferHeight()-1;

			OutputEvent e = new OutputEvent(x, y, modifiers, pointerMask);
			outputEventQueue.add(e);
			synchronized (outputEventQueue) {
				outputEventQueue.notify();
			}

			canvas.mouseX = x;
			canvas.mouseY = y;
			canvas.reDraw(); // update local pointer position
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

		if(rfbClient != 0) { // only queue if already connected

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
		rfbSetFramebufferUpdatesEnabled(framebufferUpdatesEnabled);
		return framebufferUpdatesEnabled;
	}


	void sendFramebufferUpdateRequest(int x, int y, int w, int h, boolean incremental) {
		if(framebufferUpdatesEnabled)
		{
			try {
				OutputEvent e = new OutputEvent(x, y, w, h, incremental);
				outputEventQueue.add(e);
				synchronized (outputEventQueue) {
					outputEventQueue.notify();
				}
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
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
		return ""; //TODO: wire up to native
	}


	public final String getDesktopName() {
		return rfbGetDesktopName();
	}

	public final int getFramebufferWidth() {
		return rfbGetFramebufferWidth();
	}

	public final int getFramebufferHeight() {
		return rfbGetFramebufferHeight();
	}

	public String getCutText() {
		return serverCutText;
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


	// called from native via worker thread context
	@Keep
	private void onFramebufferUpdateFinished() {

		canvas.reDraw();

		// Hide progress dialog
		if (canvas.activity.firstFrameWaitDialog.isShowing())
			canvas.handler.post(new Runnable() {
				public void run() {
					try {
						canvas.activity.firstFrameWaitDialog.dismiss();
					} catch (Exception e){
						//unused
					}
				}
			});
	}

	// called from native via worker thread context
	@Keep
	private void onGotCutText(String text) {
		serverCutText = text;
		Log.d(TAG, "got server cuttext: " + serverCutText);
	}

	// called from native via worker thread context
	@Keep
	private String onGetPassword() {
		if (connSettings.password == null || connSettings.password.length() == 0) {
			canvas.getCredsFromUser(connSettings, false); // this cares for running on the main thread
			synchronized (VNCConn.this) {
				try {
					VNCConn.this.wait();  // wait for user input to finish
				} catch (InterruptedException e) {
					//unused
				}
			}
		}
		return connSettings.password;
	}

	/**
	 * This class is used for returning user credentials from onGetCredential() to native
	 */
	@Keep
	private static class UserCredential {
		public String username;
		public String password;
	}

	// called from native via worker thread context
	@Keep
	private UserCredential onGetUserCredential() {

		if (connSettings.userName == null || connSettings.userName.isEmpty()
			   || connSettings.password == null || connSettings.password.isEmpty()) {
			canvas.getCredsFromUser(connSettings, connSettings.userName == null || connSettings.userName.isEmpty());
			synchronized (VNCConn.this) {
				try {
					VNCConn.this.wait();  // wait for user input to finish
				} catch (InterruptedException e) {
					//unused
				}
			}
		}

		UserCredential creds = new UserCredential();
		creds.username = connSettings.userName;
		creds.password = connSettings.password;
		return creds;
	}

	// called from native via worker thread context
	@Keep
	private void onNewFramebufferSize(int w, int h) {
		Log.d(TAG, "new framebuffer size " + w + " x " + h);
		// this triggers an update on what the canvas thinks about cursor position.
		// without this, the pointer highlight is off by some value after framebuffer size change
		canvas.handler.post(() -> {
			canvas.pan(0, 0);
		});
	}

}

