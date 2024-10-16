//
//  Copyright (C) 2011 Christian Beier
//  Copyright (C) 2010 Michael A. MacDonald
//  Copyright (C) 2004 Horizon Wimba.  All Rights Reserved.
//  Copyright (C) 2001-2003 HorizonLive.com, Inc.  All Rights Reserved.
//  Copyright (C) 2001,2002 Constantin Kaplinsky.  All Rights Reserved.
//  Copyright (C) 2000 Tridia Corporation.  All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
//
//  This is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This software is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this software; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//

//
// VncCanvas is a subclass of android.view.GLSurfaceView which draws a VNC
// desktop on it.
//

package com.coboltforge.dontmind.multivnc.ui;

import java.util.Arrays;
import java.util.concurrent.atomic.AtomicBoolean;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;
import javax.microedition.khronos.opengles.GL11Ext;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.opengl.GLSurfaceView;
import android.os.Handler;
import android.text.Html;
import android.util.AttributeSet;
import android.util.Base64;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.widget.EditText;
import android.widget.Toast;

import com.coboltforge.dontmind.multivnc.R;
import com.coboltforge.dontmind.multivnc.Utils;
import com.coboltforge.dontmind.multivnc.VNCConn;
import com.coboltforge.dontmind.multivnc.db.ConnectionBean;
import com.coboltforge.dontmind.multivnc.db.MetaKeyBean;
import com.coboltforge.dontmind.multivnc.db.SshKnownHost;
import com.coboltforge.dontmind.multivnc.db.VncDatabase;


public class VncCanvas extends GLSurfaceView implements VNCConn.OnFramebufferEventListener, VNCConn.OnAuthEventListener {
	static {
		System.loadLibrary("vnccanvas");
    }

	private final static String TAG = "VncCanvas";

	ZoomScaling scaling;


	// Runtime control flags
	private boolean repaintsEnabled = true;

	/**
	 * Use camera button as meta key for right mouse button
	 */
	boolean cameraButtonDown = false;

	private Dialog firstFrameWaitDialog;

	// VNC protocol connection
	public VNCConn vncConn;

	public Handler handler = new Handler();

	private PointerInputHandler inputHandler;

	private VNCGLRenderer glRenderer;

	//whether to do pointer highlighting
	boolean doPointerHighLight = true;

	/**
	 * Current state of "mouse" buttons
	 * Alt meta means use second mouse button
	 * 0 = none
	 * 1 = default button
	 * 2 = second button
	 * 4 = third button
	 */
	private int pointerMask = VNCConn.MOUSE_BUTTON_NONE;
	private int overridePointerMask = VNCConn.MOUSE_BUTTON_NONE; // this takes precedence over pointerMask if set to >= 0

	private MouseScrollRunnable scrollRunnable;


	// framebuffer coordinates of mouse pointer, Available to activity
	public int mouseX, mouseY;

	/**
	 * Position of the top left portion of the <i>visible</i> part of the screen, in
	 * full-frame coordinates
	 */
	int absoluteXPosition = 0, absoluteYPosition = 0;


	/*
		native drawing functions
	*/
    private static native void on_surface_created();
    private static native void on_surface_changed(int width, int height);
    private static native void on_draw_frame();
	private static native void prepareTexture(long rfbClient);


	private class VNCGLRenderer implements GLSurfaceView.Renderer {

		int[] textureIDs = new int[1];   // Array for 1 texture-ID
	    private int[] mTexCrop = new int[4];
	    GLShape circle;


		@Override
		public void onSurfaceCreated(GL10 gl, EGLConfig config) {

			if(Utils.DEBUG()) Log.d(TAG, "onSurfaceCreated()");

			circle = new GLShape(GLShape.CIRCLE);

			// Set color's clear-value to black
			gl.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

			/*
			 * By default, OpenGL enables features that improve quality but reduce
			 * performance. One might want to tweak that especially on software
			 * renderer.
			 */
			gl.glDisable(GL10.GL_DITHER); // Disable dithering for better performance
			gl.glDisable(GL10.GL_LIGHTING);
			gl.glDisable(GL10.GL_DEPTH_TEST);

			/*
			 * alpha blending has to be enabled manually!
			 */
			gl.glBlendFunc(GL10.GL_SRC_ALPHA, GL10.GL_ONE_MINUS_SRC_ALPHA);

			/*
			 * setup texture stuff
			 */
			// Enable 2d textures
			gl.glEnable(GL10.GL_TEXTURE_2D);
			// Generate texture-ID array
			gl.glDeleteTextures(1, textureIDs, 0);
			gl.glGenTextures(1, textureIDs, 0);
			 // this is a 2D texture
			gl.glBindTexture(GL10.GL_TEXTURE_2D, textureIDs[0]);
			// Set up texture filters --> nice smoothing
			gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_LINEAR);
			gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_LINEAR);
		}

		// Call back after onSurfaceCreated() or whenever the window's size changes
		@Override
		public void onSurfaceChanged(GL10 gl, int width, int height) {

			if(Utils.DEBUG()) Log.d(TAG, "onSurfaceChanged()");

			// Set the viewport (display area) to cover the entire window
			gl.glViewport(0, 0, width, height);

			// Setup orthographic projection
			gl.glMatrixMode(GL10.GL_PROJECTION); // Select projection matrix
			gl.glLoadIdentity();                 // Reset projection matrix
			gl.glOrthox(0, width, height, 0, 0, 100);


			gl.glMatrixMode(GL10.GL_MODELVIEW);  // Select model-view matrix
			gl.glLoadIdentity();                 // Reset
		}

		@Override
		public void onDrawFrame(GL10 gl) {

			// TODO optimize: texSUBimage ?
			// pbuffer: http://blog.shayanjaved.com/2011/05/13/android-opengl-es-2-0-render-to-texture/

			try{
				gl.glClear(GL10.GL_COLOR_BUFFER_BIT | GL10.GL_DEPTH_BUFFER_BIT);

				if(vncConn.getFramebufferWidth() >0 && vncConn.getFramebufferHeight() > 0) {
					vncConn.lockFramebuffer();
					prepareTexture(vncConn.rfbClient);
					vncConn.unlockFramebuffer();
				}

				/*
				 * The crop rectangle is given as Ucr, Vcr, Wcr, Hcr.
				 * That is, "left"/"bottom"/width/height, although you can
				 * also have negative width and height to flip the image.
				 *
				 * This is the part of the framebuffer we show on-screen.
				 *
				 * If absolute[XY]Position are negative that means the framebuffer
				 * is smaller than our viewer window.
				 *
				 */
				mTexCrop[0] = absoluteXPosition >= 0 ? absoluteXPosition : 0; // don't let this be <0
				mTexCrop[1] = absoluteYPosition >= 0 ? (int)(absoluteYPosition + VncCanvas.this.getHeight() / getScale()) : vncConn.getFramebufferHeight();
				mTexCrop[2] = (int) (VncCanvas.this.getWidth() < vncConn.getFramebufferWidth()*getScale() ? VncCanvas.this.getWidth() / getScale() : vncConn.getFramebufferWidth());
				mTexCrop[3] = (int) -(VncCanvas.this.getHeight() < vncConn.getFramebufferHeight()*getScale() ? VncCanvas.this.getHeight() / getScale() : vncConn.getFramebufferHeight());

				if(Utils.DEBUG()) Log.d(TAG, "cropRect: u:" + mTexCrop[0] + " v:" + mTexCrop[1] + " w:" + mTexCrop[2] + " h:" + mTexCrop[3]);

				((GL11) gl).glTexParameteriv(GL10.GL_TEXTURE_2D, GL11Ext.GL_TEXTURE_CROP_RECT_OES, mTexCrop, 0);

				/*
				 * Very fast, but very basic transforming: only transpose, flip and scale.
				 * Uses the GL_OES_draw_texture extension to draw sprites on the screen without
				 * any sort of projection or vertex buffers involved.
				 *
				 * See http://www.khronos.org/registry/gles/extensions/OES/OES_draw_texture.txt
				 *
				 * All parameters in GL screen coordinates!
				 */
				int x = (int) (VncCanvas.this.getWidth() < vncConn.getFramebufferWidth()*getScale() ? 0 : VncCanvas.this.getWidth()/2 - (vncConn.getFramebufferWidth()*getScale())/2);
				int y = (int) (VncCanvas.this.getHeight() < vncConn.getFramebufferHeight()*getScale() ? 0 : VncCanvas.this.getHeight()/2 - (vncConn.getFramebufferHeight()*getScale())/2);
				int w = (int) (VncCanvas.this.getWidth() < vncConn.getFramebufferWidth()*getScale() ? VncCanvas.this.getWidth(): vncConn.getFramebufferWidth()*getScale());
				int h =(int) (VncCanvas.this.getHeight() < vncConn.getFramebufferHeight()*getScale() ? VncCanvas.this.getHeight(): vncConn.getFramebufferHeight()*getScale());
				gl.glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // opaque!
				((GL11Ext) gl).glDrawTexfOES(x, y, 0, w, h);

				if(Utils.DEBUG()) Log.d(TAG, "drawing to screen: x " + x + " y " + y + " w " + w + " h " + h);


				/*
				 * do pointer highlight overlay
				 */
				if(doPointerHighLight) {
					gl.glEnable(GL10.GL_BLEND);
					int mouseXonScreen = (int)(getScale()*(mouseX-absoluteXPosition));
					int mouseYonScreen = (int)(getScale()*(mouseY-absoluteYPosition));

					gl.glLoadIdentity();                 // Reset model-view matrix
					gl.glTranslatex( mouseXonScreen, mouseYonScreen, 0);
					gl.glColor4f(1.0f, 0.2f, 1.0f, 0.1f);

					// simulate some anti-aliasing by drawing the shape 3x
					gl.glScalef(0.001f, 0.001f, 0.0f);
					circle.draw(gl);

					gl.glScalef(0.99f, 0.99f, 0.0f);
					circle.draw(gl);

					gl.glScalef(0.99f, 0.99f, 0.0f);
					circle.draw(gl);

					gl.glDisable(GL10.GL_BLEND);
				}

			}
			catch(NullPointerException e) {
			}

		}

	}


	/**
	 * Constructor used by the inflation apparatus
	 * @param context
	 */
	public VncCanvas(final Context context, AttributeSet attrs)
	{
		super(context, attrs);
		scrollRunnable = new MouseScrollRunnable();

		setFocusable(true);

		glRenderer = new VNCGLRenderer();
		setRenderer(glRenderer);
		// only render upon request
		setRenderMode(RENDERMODE_WHEN_DIRTY);

		int oldprio = android.os.Process.getThreadPriority(android.os.Process.myTid());
		// GIVE US MAGIC POWER, O GREAT FAIR SCHEDULER!
		android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_DISPLAY);
		Log.d(TAG, "Changed prio from " + oldprio + " to " + android.os.Process.getThreadPriority(android.os.Process.myTid()));
	}

	/**
	 * Create a view showing a VNC connection
	 */
	void initializeVncCanvas(Dialog progressDialog, PointerInputHandler inputHandler, VNCConn conn) {
		firstFrameWaitDialog = progressDialog;
		this.inputHandler = inputHandler;
		vncConn = conn;
	}

	/**
	 * Warp the mouse to x, y in the RFB coordinates
	 * @param x
	 * @param y
	 */
	public void warpMouse(int x, int y)
	{
		mouseX=vncConn.trimX(x);
		mouseY=vncConn.trimY(y);
		reDraw(); // update local pointer position
		panToMouse();
		vncConn.sendPointerEvent(x, y, 0, VNCConn.MOUSE_BUTTON_NONE);
	}


	private void mouseFollowPan()
	{
		try {
			if (vncConn.getConnSettings().followPan)
			{
				int scrollx = absoluteXPosition;
				int scrolly = absoluteYPosition;
				int width = getVisibleWidth();
				int height = getVisibleHeight();
				//Log.i(TAG,"scrollx " + scrollx + " scrolly " + scrolly + " mouseX " + mouseX +" Y " + mouseY + " w " + width + " h " + height);
				if (mouseX < scrollx || mouseX >= scrollx + width || mouseY < scrolly || mouseY >= scrolly + height)
				{
					//Log.i(TAG,"warp to " + scrollx+width/2 + "," + scrolly + height/2);
					warpMouse(scrollx + width/2, scrolly + height / 2);
				}
			}}
		catch(NullPointerException e) {
		}
	}


	/**
	 * Apply scroll offset and scaling to convert touch-space coordinates to the corresponding
	 * point on the full frame.
	 * @param e MotionEvent with the original, touch space coordinates.  This event is altered in place.
	 * @return e -- The same event passed in, with the coordinates mapped
	 */
	MotionEvent changeTouchCoordinatesToFullFrame(MotionEvent e)
	{
		//Log.v(TAG, String.format("tap at %f,%f", e.getX(), e.getY()));
		float scale = getScale();

		// Adjust coordinates for Android notification bar.
		e.offsetLocation(0, -1f * getTop());

		e.setLocation(absoluteXPosition + e.getX() / scale, absoluteYPosition + e.getY() / scale);

		return e;
	}

	public void onDestroy() {
		Log.v(TAG, "Cleaning up resources");
		try {
			vncConn.shutdown();
		}
		catch(NullPointerException e) {
		}
		vncConn = null;
	}

	@Override
	public void onPause() {
		/*
		 * this is to avoid a deadlock between GUI thread and GLThread:
		 *
		 * the GUI thread would call onPause on the GLThread which would never return since
		 * the GL thread's GLThreadManager object is waiting on the GLThread.
		 */
		try {
			vncConn.unlockFramebuffer();
		}
		catch(IllegalMonitorStateException e) {
			// thrown when mutex was not locked
		}
		catch(NullPointerException e) {
			// thrown if we fatal out at the very beginning
		}

		super.onPause();
	}

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);

        // Synchronize absoluteXPosition & absoluteYPosition with new size.
        // We use pan() with delta 0 to trigger their re-calculation.
        //
        // oldw & oldh will be 0 if we are just now added to activity.
        if (oldw != 0 || oldh != 0) {
            pan(0, 0);
        }
    }


	/*
	 * f(x,s) is a function that returns the coordinate in screen/scroll space corresponding
	 * to the coordinate x in full-frame space with scaling s.
	 *
	 * This function returns the difference between f(x,s1) and f(x,s2)
	 *
	 * f(x,s) = (x - i/2) * s + ((i - w)/2)) * s
	 *        = s (x - i/2 + i/2 + w/2)
	 *        = s (x + w/2)
	 *
	 *
	 * f(x,s) = (x - ((i - w)/2)) * s
	 * @param oldscaling
	 * @param scaling
	 * @param imageDim
	 * @param windowDim
	 * @param offset
	 * @return
	 */



	/**
	 * Make sure mouse is visible on displayable part of screen
	 */
	public void panToMouse()
	{
		try {
			if (!vncConn.getConnSettings().followMouse)
				return;
		} catch (NullPointerException e) {
			return;
		}

		int x = mouseX;
		int y = mouseY;
		int w = getVisibleWidth();
		int h = getVisibleHeight();
		int iw = vncConn.getFramebufferWidth();
		int ih = vncConn.getFramebufferHeight();

		int newX = absoluteXPosition;
		int newY = absoluteYPosition;

		if (x - newX >= w - 5)
		{
			newX = x - w + 5;
			if (newX + w > iw)
				newX = iw - w;
		}
		else if (x < newX + 5)
		{
			newX = x - 5;
			if (newX < 0)
				newX = 0;
		}
		if ( newX != absoluteXPosition ) {
			absoluteXPosition = newX;
		}
		if (y - newY >= h - 5)
		{
			newY = y - h + 5;
			if (newY + h > ih)
				newY = ih - h;
		}
		else if (y < newY + 5)
		{
			newY = y - 5;
			if (newY < 0)
				newY = 0;
		}
		if ( newY != absoluteYPosition ) {
			absoluteYPosition = newY;
		}
	}

	/**
	 * Pan by a number of pixels (relative pan)
	 * @param dX
	 * @param dY
	 * @return True if the pan changed the view (did not move view out of bounds); false otherwise
	 */
	public boolean pan(int dX, int dY) {

		try {
			double scale = getScale();

			double sX = (double) dX / scale;
			double sY = (double) dY / scale;

			if (absoluteXPosition + sX < 0)
				// dX = diff to 0
				sX = -absoluteXPosition;
			if (absoluteYPosition + sY < 0)
				sY = -absoluteYPosition;

			// Prevent panning right or below desktop image
			if (absoluteXPosition + getVisibleWidth() + sX > vncConn.getFramebufferWidth())
				sX = vncConn.getFramebufferWidth() - getVisibleWidth() - absoluteXPosition;
			if (absoluteYPosition + getVisibleHeight() + sY > vncConn.getFramebufferHeight())
				sY = vncConn.getFramebufferHeight() - getVisibleHeight() - absoluteYPosition;

			absoluteXPosition += sX;
			absoluteYPosition += sY;

			// whene the frame buffer is smaller than the view,
			// it is is centered!
			if (vncConn.getFramebufferWidth() < getVisibleWidth())
				absoluteXPosition /= 2;
			if (vncConn.getFramebufferHeight() < getVisibleHeight())
				absoluteYPosition /= 2;


			if (sX != 0.0 || sY != 0.0) {
				return true;
			}
		} catch (Exception ignored) {
		}
		return false;
	}

	/* (non-Javadoc)
	 * @see android.view.View#onScrollChanged(int, int, int, int)
	 */
	@Override
	protected void onScrollChanged(int l, int t, int oldl, int oldt) {
		super.onScrollChanged(l, t, oldl, oldt);
		try {
			mouseFollowPan();
		}
		catch(NullPointerException e) {
		}
	}



	@Override
	public boolean onTouchEvent(MotionEvent event) {
		return inputHandler.onTouchEvent(event);
	}


	@Override
	public boolean onGenericMotionEvent(MotionEvent event) {
		return inputHandler.onGenericMotionEvent(event);
	}

	public void reDraw() {

		if (repaintsEnabled && vncConn.rfbClient != 0) {
			// request a redraw from GL thread
			requestRender();
		}
	}

	public void disableRepaints() {
		repaintsEnabled = false;
	}

	public void enableRepaints() {
		repaintsEnabled = true;
	}

	public void setPointerHighlight(boolean enable) {
		doPointerHighLight = enable;
	}

	public final boolean getPointerHighlight() {
		return doPointerHighLight;
	}



	public void showConnectionInfo() {
		try {
			String msg = vncConn.getDesktopName();
			msg += "(" + (vncConn.isEncrypted() ? getContext().getString(R.string.encrypted) : getContext().getString(R.string.unencrypted)) + ")";
			int idx = vncConn.getDesktopName().indexOf("(");
			if (idx > -1) {
				// Breakup actual desktop name from IP addresses for improved
				// readability
				String dn = vncConn.getDesktopName().substring(0, idx).trim();
				String ip = vncConn.getDesktopName().substring(idx).trim();
				msg = dn + "\n" + ip;
			}
			msg += "\n" + vncConn.getFramebufferWidth() + "x" + vncConn.getFramebufferHeight();
			String enc = vncConn.getEncoding();
			// Encoding might not be set when we display this message
			if (enc != null && !enc.equals(""))
				msg += ", " + vncConn.getEncoding() + " encoding, " + vncConn.getColorModel().toString();
			else
				msg += ", " + vncConn.getColorModel().toString();
			Toast.makeText(getContext(), msg, Toast.LENGTH_LONG).show();
		}
		catch(Exception ignored) {
		}
	}


	/** Set which mouse buttons are down.
	 * @param pointerMask
	 */
	public void setOverridePointerMask(int pointerMask) {
		overridePointerMask = pointerMask;
	}


	/**
	 * Convert a motion event to a format suitable for sending over the wire
	 * @param evt motion event; x and y must already have been converted from screen coordinates
	 * to remote frame buffer coordinates.  cameraButton flag is interpreted as second mouse
	 * button
	 * @param downEvent True if "mouse button" (touch or trackball button) is down when this happens
	 * @return true if event was actually sent
	 */
	public boolean processPointerEvent(MotionEvent evt,boolean downEvent)
	{
		return processPointerEvent(evt,downEvent,cameraButtonDown);
	}

	/**
	 * Convert a motion event to a format suitable for sending over the wire
	 * @param evt motion event; x and y must already have been converted from screen coordinates
	 * to remote frame buffer coordinates.
	 * @param mouseIsDown True if "mouse button" (touch or trackball button) is down when this happens
	 * @param useRightButton If true, event is interpreted as happening with right mouse button
	 * @return true if event was actually sent
	 */
	public boolean processPointerEvent(MotionEvent evt,boolean mouseIsDown,boolean useRightButton) {
		try {
			int action = evt.getAction();
			 if (action == MotionEvent.ACTION_DOWN || (mouseIsDown && action == MotionEvent.ACTION_MOVE)) {
			      if (useRightButton) {
			    	  if(action == MotionEvent.ACTION_MOVE)
			    		  if(Utils.DEBUG()) Log.d(TAG, "Input: moving, right mouse button down");
			    	  else
			    		  if(Utils.DEBUG()) Log.d(TAG, "Input: right mouse button down");

			    	  pointerMask = VNCConn.MOUSE_BUTTON_RIGHT;
			      } else {
			    	  if(action == MotionEvent.ACTION_MOVE)
			    		  if(Utils.DEBUG()) Log.d(TAG, "Input: moving, left mouse button down");
			    	  else
			    		  if(Utils.DEBUG()) Log.d(TAG, "Input: left mouse button down");

			    	  pointerMask = VNCConn.MOUSE_BUTTON_LEFT;
			      }
			    } else if (action == MotionEvent.ACTION_UP) {
			    	if(Utils.DEBUG()) Log.d(TAG, "Input: all mouse buttons up");
			    	pointerMask = 0;
			    }
			 mouseX = vncConn.trimX((int)evt.getX());
			 mouseY = vncConn.trimY((int)evt.getY());
			 reDraw(); // update local pointer position
			 panToMouse();
			 if(overridePointerMask > 0)
				 return vncConn.sendPointerEvent((int)evt.getX(),(int)evt.getY(), evt.getMetaState(), overridePointerMask);
			 else
				 return vncConn.sendPointerEvent((int)evt.getX(),(int)evt.getY(), evt.getMetaState(), pointerMask);

		}
		catch(NullPointerException e) {
			return false;
		}
	}

	/**
	 * Process given mouse event.
	 *
	 * @param button Which button is pressed/released (0 for no button)
	 * @param isDown True if button is down
	 * @param x,y    Event position in remote frame buffer coordinates.
	 */
	public void processMouseEvent(int button, boolean isDown, int x, int y) {
		try {
			if (isDown)
				pointerMask |= button;
			else
				pointerMask &= ~button;

			mouseX = vncConn.trimX(x);
			mouseY = vncConn.trimY(y);
			reDraw(); // update local pointer position
			panToMouse();
			if (overridePointerMask > 0)
				vncConn.sendPointerEvent(x, y, 0, overridePointerMask);
			else
				vncConn.sendPointerEvent(x, y, 0, pointerMask);

		} catch (NullPointerException ignored) {
		}
	}



	/**
	 * Moves the scroll while the volume key is held down
	 * @author Michael A. MacDonald
	 */
	class MouseScrollRunnable implements Runnable
	{
		int delay = 100;

		int scrollButton = 0;

		/* (non-Javadoc)
		 * @see java.lang.Runnable#run()
		 */
		@Override
		public void run() {
			vncConn.sendPointerEvent(mouseX, mouseY, 0, scrollButton);
			vncConn.sendPointerEvent(mouseX, mouseY, 0, 0);

			handler.postDelayed(this, delay);
		}
	}

	public boolean processLocalKeyEvent(int keyCode, KeyEvent evt) {

		// prevent actionbar from stealing focus on key down
		requestFocus();

		if (keyCode == KeyEvent.KEYCODE_MENU)
			// Ignore menu key
			return true;
		if (keyCode == KeyEvent.KEYCODE_CAMERA)
		{
			cameraButtonDown = (evt.getAction() != KeyEvent.ACTION_UP);
		}
		else if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN || keyCode == KeyEvent.KEYCODE_VOLUME_UP)
		{
			int mouseChange = keyCode == KeyEvent.KEYCODE_VOLUME_DOWN ? VNCConn.MOUSE_BUTTON_SCROLL_DOWN : VNCConn.MOUSE_BUTTON_SCROLL_UP;
			if (evt.getAction() == KeyEvent.ACTION_DOWN)
			{
				// If not auto-repeat
				if (scrollRunnable.scrollButton != mouseChange)
				{
					pointerMask |= mouseChange;
					scrollRunnable.scrollButton = mouseChange;
					handler.postDelayed(scrollRunnable,200);
				}
			}
			else
			{
				handler.removeCallbacks(scrollRunnable);
				scrollRunnable.scrollButton = 0;
				pointerMask &= ~mouseChange;
			}

			vncConn.sendPointerEvent(mouseX, mouseY, evt.getMetaState(), pointerMask);

			return true;
		}

		return vncConn.sendKeyEvent(keyCode, evt, false);
	}

	void sendMetaKey(MetaKeyBean meta)
	{
		Log.d(TAG, "sendMetaKey " + (meta != null ? meta.getKeyDesc() : "none"));

		if(meta == null)
			return;

		if (meta.isMouseClick)
		{
			vncConn.sendPointerEvent(mouseX, mouseY, meta.metaFlags, meta.mouseButtons);
			vncConn.sendPointerEvent(mouseX, mouseY, meta.metaFlags, 0);
		}
		else {
			// KeyEvent(downTime, eventTime, action, code, repeat, metaState)
			KeyEvent downEvent = new KeyEvent(
					System.currentTimeMillis(),
					System.currentTimeMillis(),
					KeyEvent.ACTION_DOWN,
					meta.keySym,
					0,
					meta.metaFlags);
			vncConn.sendKeyEvent(downEvent.getKeyCode(), downEvent, true);

			// and up again
			KeyEvent upEvent = KeyEvent.changeAction(downEvent, KeyEvent.ACTION_UP);
			vncConn.sendKeyEvent(upEvent.getKeyCode(), upEvent, true);

		}
	}

	float getScale()
	{
		if (scaling == null)
			return 1;
		return scaling.getScale();
	}

	/**
	 *
	 * @return The smallest scale supported by the implementation; the scale at which
	 * the bitmap would be smaller than the screen
	 */
	float getMinimumScale()
	{
		double scale = 0.75;
		int displayWidth = getWidth();
		int displayHeight = getHeight();
		for (; scale >= 0; scale -= 0.25)
		{
			try {
				if (scale * vncConn.getFramebufferWidth() < displayWidth && scale * vncConn.getFramebufferHeight() < displayHeight)
					break;
			} catch (NullPointerException ignored) {
				break;
			}
		}
		return (float)(scale + 0.25);
	}

	public int getVisibleWidth() {
		return (int)((double)getWidth() / getScale() + 0.5);
	}

	public int getVisibleHeight() {
		return (int)((double)getHeight() / getScale() + 0.5);
	}

	@Override
	public void onFramebufferUpdateFinished() {
		reDraw();

		// Hide progress dialog
		if (firstFrameWaitDialog.isShowing())
			handler.post(() -> {
				try {
					firstFrameWaitDialog.dismiss();
				} catch (Exception e){
					//unused
				}
			});
	}

	@Override
	public void onNewFramebufferSize(int w, int h) {
		// this triggers an update on what the canvas thinks about cursor position.
		// without this, the pointer highlight is off by some value after framebuffer size change
		handler.post(() -> {
			try {
				pan(0, 0);
			} catch (Exception ignored) {
			}
		});
	}

	@Override
	public void onRequestCredsFromUser(final ConnectionBean c, boolean isUserNameNeeded) {
		// this method is probably called from the vnc thread
		post(new Runnable() {
			@Override
			public void run() {

				LayoutInflater layoutinflater = LayoutInflater.from(getContext());
				View credentialsDialog = layoutinflater.inflate(R.layout.credentials_dialog, null);
				if(!isUserNameNeeded)
					credentialsDialog.findViewById(R.id.username_row).setVisibility(GONE);

				AlertDialog dialog = new AlertDialog.Builder(getContext())
						.setTitle(getContext().getString(R.string.credentials_needed_title))
						.setView(credentialsDialog)
						.setCancelable(false)
						.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog, int whichButton) {
								if(isUserNameNeeded)
									c.userName = ((EditText)credentialsDialog.findViewById(R.id.userName)).getText().toString();
								c.password = ((EditText)credentialsDialog.findViewById(R.id.password)).getText().toString();
								synchronized (vncConn) {
									vncConn.notify();
								}
							}
						}).create();

				dialog.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_VISIBLE);
				dialog.show();
			}
		});

	}

	@Override
	public void onRequestSshFingerprintCheck(String host, byte[] fingerprint, AtomicBoolean doContinue) {
		// this method is probably called from the vnc thread
		post(() -> {
			// look for host, if not found create entry and return ok
			SshKnownHost knownHost = VncDatabase.getInstance(getContext()).getSshKnownHostDao().get(host);
			if(knownHost == null) {
				AlertDialog dialog = new AlertDialog.Builder(getContext())
						.setTitle(R.string.ssh_key_new_title)
						// always using SHA256 in native part, ok to hardcode this here
						.setMessage(Html.fromHtml(getContext().getString(R.string.ssh_key_new_message,
								"SHA256:" + Base64.encodeToString(fingerprint,Base64.NO_PADDING|Base64.NO_WRAP))))
						.setCancelable(false)
						.setPositiveButton(R.string.ssh_key_new_continue, (dialog12, whichButton) -> {
							VncDatabase.getInstance(getContext()).getSshKnownHostDao().insert(new SshKnownHost(0, host, fingerprint));
							doContinue.set(true);
							synchronized (vncConn) {
								vncConn.notify();
							}
						})
						.setNegativeButton(R.string.ssh_key_new_abort, (dialog1, whichButton) -> {
							doContinue.set(false);
							synchronized (vncConn) {
								vncConn.notify();
							}
						})
						.create();

				dialog.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_VISIBLE);
				dialog.show();
				return;
			}

			// host found, check if fingerprint matches
			if(Arrays.equals(knownHost.fingerprint, fingerprint)) {
				doContinue.set(true);
				synchronized (vncConn) {
					vncConn.notify();
				}
			} else {
				// not matching, ask user!
				AlertDialog dialog = new AlertDialog.Builder(getContext())
						.setTitle(R.string.ssh_key_mismatch_title)
						// always using SHA256 in native part, ok to hardcode this here
						.setMessage(Html.fromHtml(getContext().getString(R.string.ssh_key_mismatch_message,
								"SHA256:" + Base64.encodeToString(fingerprint,Base64.NO_PADDING|Base64.NO_WRAP))))
						.setCancelable(false)
						.setPositiveButton(R.string.ssh_key_mismatch_continue, (dialog12, whichButton) -> {
							SshKnownHost updatedHost = new SshKnownHost(knownHost.id, host, fingerprint);
							VncDatabase.getInstance(getContext()).getSshKnownHostDao().update(updatedHost);
							doContinue.set(true);
							synchronized (vncConn) {
								vncConn.notify();
							}
						})
						.setNegativeButton(R.string.ssh_key_mismatch_abort, (dialog1, whichButton) -> {
							doContinue.set(false);
							synchronized (vncConn) {
								vncConn.notify();
							}
						})
						.create();

				dialog.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_VISIBLE);
				dialog.show();
			}
		});
	}

	@Override
	public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
		Log.d(TAG, "onCreateInputConnection"); // only called when focusableInTouchMode==true
		outAttrs.imeOptions = EditorInfo.IME_FLAG_NO_FULLSCREEN; // need to set this for some keyboards in landscape mode
		return super.onCreateInputConnection(outAttrs);
	}
}
