/**
 *
 * This represents an *active* VNC connection (as opposed to ConnectionBean, which is more like a bookmark.).
 * Copyright (C) 2012-2023 Christian Beier <info@christianbeier.net>
 */


package com.coboltforge.dontmind.multivnc;

import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import android.util.Log;
import android.view.KeyEvent;
import androidx.annotation.Keep;

import com.coboltforge.dontmind.multivnc.db.ConnectionBean;


public class VNCConn {


	public interface OnInitListener {
		/**
		 * Fired on connection init.
		 * @param error Null on successful connection init (i.e. connected), non-null on failure.
		 */
		void onInit(Throwable error);
	}

	public interface OnDisconnectListener {
		/**
		 * Fired on disconnect, either orderly or with error
		 * @param err Null on orderly disconnect, non-null on failure.
		 */
		void onDisconnected(Throwable err);
	}

	public interface OnFramebufferEventListener {
		void onFramebufferUpdateFinished();
		void onNewFramebufferSize(int w, int h);
	}

	public interface OnAuthEventListener {
		void onRequestCredsFromUser(final ConnectionBean conn, boolean isUserNameNeeded);
		void onRequestSshFingerprintCheck(String host, byte[] fingerprint, final AtomicBoolean doContinue);
	}

	private final OnFramebufferEventListener onFramebufferEventCallback;
	private final OnAuthEventListener onAuthEventCallback;

	static {
		System.loadLibrary("vncconn");
	}

	private final static String TAG = "VNCConn";

	private ServerToClientThread inputThread;
	private ClientToServerThread outputThread;

	// the native rfbClient
	@Keep
	public long rfbClient;
	private ConnectionBean connSettings;
	private COLORMODEL pendingColorModel = COLORMODEL.C24bit;

	// Runtime control flags
	private boolean maintainConnection = true;
	private boolean framebufferUpdatesEnabled = true;

	private final Lock bitmapDataPixelsLock = new ReentrantLock();

	// message queue for communicating with the output worker thread
	private final ConcurrentLinkedQueue<OutputEvent> outputEventQueue = new ConcurrentLinkedQueue<>();

	private COLORMODEL colorModel;

	private String serverCutText;

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

		private final OnInitListener initCallback;
		private final OnDisconnectListener disconnectCallback;

    	public ServerToClientThread(OnInitListener initCallback, OnDisconnectListener disconnectCallback) {
			this.initCallback = initCallback;
    		this.disconnectCallback = disconnectCallback;
    	}


		public void run() {

			if(Utils.DEBUG()) Log.d(TAG, "ServerToClientThread started!");

			Throwable maybeError = null;

			try {

				/*
				 * if IPv6 address, add scope id
				 */
				try {
					InetAddress address = InetAddress.getByName(connSettings.address);

					Inet6Address in6 = Inet6Address.getByAddress(
							address.getHostName(),
							address.getAddress(),
							Utils.getActiveNetworkInterface());

					connSettings.address = in6.getHostAddress();
					Log.i(TAG, "Using IPv6");

				} catch (UnknownHostException unused) {
					Log.i(TAG, "Using IPv4");
				} catch (NullPointerException ne) {
					Log.e(TAG, ne.toString());
				}


				int repeaterId = (connSettings.useRepeater && connSettings.repeaterId != null && connSettings.repeaterId.length() > 0) ? Integer.parseInt(connSettings.repeaterId) : -1;
				lockFramebuffer();

				final COMPRESSMODEL compressModel = COMPRESSMODEL.valueOf(connSettings.compressModel);
				final QUALITYMODEL qualityModel = QUALITYMODEL.valueOf(connSettings.qualityModel);
				final boolean enableCompress = COMPRESSMODEL.None != compressModel;
				final boolean enableJPEG = QUALITYMODEL.None != qualityModel;
        if (!rfbInit(connSettings.address, connSettings.port, repeaterId, pendingColorModel.bpp(),
	          connSettings.encodingsString,
						enableCompress,
						enableJPEG,
						COMPRESSMODEL.valueOf(connSettings.compressModel).toParameter(),
						QUALITYMODEL.valueOf(connSettings.qualityModel).toParameter(),     
						connSettings.sshHost,
						connSettings.sshUsername,
						connSettings.sshPassword,
						connSettings.sshPrivkey,
						connSettings.sshPrivkeyPassword
				)) {
					unlockFramebuffer();
					throw new Exception(); //TODO add some error reoprting here, e.g. if auth fail or not
				}
				colorModel = pendingColorModel;
				unlockFramebuffer();

				// update connection's nickname with desktop name if unset
				if(connSettings.nickname == null || connSettings.nickname.isEmpty())
					connSettings.nickname = getDesktopName();

				// start output thread here
				outputThread = new ClientToServerThread();
				outputThread.start();

				initCallback.onInit(null);

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

					// save for callback
					maybeError = e;
				}
			}

			// we might get here when maintainConnection is set to false or when an exception was thrown
			lockFramebuffer(); // make sure the native texture drawing is not accessing something invalid
			rfbShutdown();
			unlockFramebuffer();

			if(outputThread == null) {
				// if outputThread is still unset, we were in init
				initCallback.onInit(maybeError);
			} else {
				disconnectCallback.onDisconnected(maybeError);
			}

			if(Utils.DEBUG()) Log.d(TAG, "ServerToClientThread done!");
		}

    }




    private class ClientToServerThread extends Thread {

    	public void run() {

			if(Utils.DEBUG()) {
				Log.d(TAG, "ClientToServerThread started!");
			}

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
			boolean status = false;
			if (rfbClient != 0) {
				// first, try sending UTF-8
				if(rfbSendClientCutTextUTF8(text)) {
					if (Utils.DEBUG()) Log.d(TAG, "Sent UTF-8 cuttext: '"  + text + "' successfully");
					status = true;
				} else {
					// server does not support Extended Clipboard, try Latin-1.
					// encode() docs say: "This method always replaces malformed-input and unmappable-character
					// sequences with this charset's default replacement string."
					status = rfbSendClientCutText(StandardCharsets.ISO_8859_1.encode(text).array());
					if (Utils.DEBUG()) Log.d(TAG, "Sent ISO-8859-1 cuttext: '"  + text + "'" + (status ? "successfully" : "unsuccessfully"));
				}
			}
			return status;
		}


    }


	private native boolean rfbInit(String host, int port, int repeaterId, int bytesPerPixel,
                   String encodingsString, boolean hasCompress, boolean enableJPEG, int compressLevel, int qualityLevel,
								   String ssh_host,
								   String ssh_user,
								   String ssh_password,
								   byte[] ssh_priv_key,
								   String ssh_priv_key_password);

  private native void rfbShutdown();
	private native boolean rfbProcessServerMessage();
	private native boolean rfbSetFramebufferUpdatesEnabled(boolean enable);
	private native String rfbGetDesktopName();
	private native int rfbGetFramebufferWidth();
	private native int rfbGetFramebufferHeight();
	private native boolean rfbSendKeyEvent(long keysym, boolean down);
	private native boolean rfbSendPointerEvent(int x, int y, int buttonMask);
	private native boolean rfbSendClientCutText(byte[] bytes);
	private native boolean rfbSendClientCutTextUTF8(String text);
	private native boolean rfbIsEncrypted();


	public VNCConn(OnFramebufferEventListener framebufferCallback, OnAuthEventListener authCallback) {
		onFramebufferEventCallback = framebufferCallback;
		onAuthEventCallback = authCallback;
		if(Utils.DEBUG()) Log.d(TAG, this + " constructed!");
	}

	protected void finalize() {
		if(Utils.DEBUG()) Log.d(TAG, this + " finalized!");
	}


	/**
	 * Initialise a VNC connection
	 * @param bean Connection settings
	 * @param initCallback Callback that's called after init has succeeded or failed
	 * @param disconnectCallback Callback that's called when an established connection disconnects
	 */
	public void init(ConnectionBean bean, OnInitListener initCallback, OnDisconnectListener disconnectCallback) {

		Log.d(TAG, "initializing");

		maintainConnection = true;

		connSettings = bean;
		this.pendingColorModel = COLORMODEL.valueOf(bean.colorModel);

		inputThread = new ServerToClientThread(initCallback, disconnectCallback);
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

		connSettings = null;

		System.gc();
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

			// safety: trim coordinates
			x = trimX(x);
			y = trimY(y);

			OutputEvent e = new OutputEvent(x, y, modifiers, pointerMask);
			outputEventQueue.add(e);
			synchronized (outputEventQueue) {
				outputEventQueue.notify();
			}

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
					case KeyEvent.KEYCODE_SHIFT_LEFT:   keyCode = 0xffe1; break;
					case KeyEvent.KEYCODE_SHIFT_RIGHT:  keyCode = 0xffe2; break;							
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

	public int trimX(int x) {
		if (x<0) return 0;
		else if (x>=getFramebufferWidth()) return getFramebufferWidth()-1;
		return x;
	}

	public int trimY(int y) {
		if (y<0) return 0;
		else if (y>=getFramebufferHeight()) return getFramebufferHeight()-1;
		return y;
	}

	public String getCutText() {
		return serverCutText;
	}

	public boolean isEncrypted() { return rfbIsEncrypted(); }

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
		if(onFramebufferEventCallback != null)
			onFramebufferEventCallback.onFramebufferUpdateFinished();
	}

	// called from native via worker thread context
	@Keep
	private void onGotCutText(byte[] bytes) {
		serverCutText = StandardCharsets.ISO_8859_1.decode(ByteBuffer.wrap(bytes)).toString();
		Log.d(TAG, "got ISO-8859-1 server cuttext: " + serverCutText);
	}

	// called from native via worker thread context
	@Keep
	private void onGotCutTextUTF8(String text) {
		serverCutText = text;
		Log.d(TAG, "got UTF-8 server cuttext: " + serverCutText);
	}

	// called from native via worker thread context
	@Keep
	private String onGetPassword() {
		if (onAuthEventCallback != null && (connSettings.password == null || connSettings.password.length() == 0)) {
			onAuthEventCallback.onRequestCredsFromUser(connSettings, false); // this cares for running on the main thread
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

		if (onAuthEventCallback != null &&
				(connSettings.userName == null || connSettings.userName.isEmpty()
			   || connSettings.password == null || connSettings.password.isEmpty())) {
			onAuthEventCallback.onRequestCredsFromUser(connSettings, connSettings.userName == null || connSettings.userName.isEmpty());
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
		if(onFramebufferEventCallback != null)
			onFramebufferEventCallback.onNewFramebufferSize(w, h);
	}

	// called from native via worker thread context
	@Keep
	private int onSshFingerprintCheck(String host, byte[] fingerprint) {
		if(onAuthEventCallback != null) {
			AtomicBoolean doContinue = new AtomicBoolean();
			onAuthEventCallback.onRequestSshFingerprintCheck(host, fingerprint, doContinue);
			synchronized (VNCConn.this) {
				try {
					VNCConn.this.wait();  // wait for user input to finish
				} catch (InterruptedException e) {
					//unused
				}
			}
			// evaluate result
			if(doContinue.get()) {
				return 0;
			}
		}
		return -1;
	}
}

