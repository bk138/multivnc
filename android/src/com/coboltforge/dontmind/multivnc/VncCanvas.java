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

package com.coboltforge.dontmind.multivnc;

import java.nio.IntBuffer;
import java.util.concurrent.atomic.AtomicBoolean;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;
import javax.microedition.khronos.opengles.GL11Ext;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Matrix;
import android.opengl.GLSurfaceView;
import android.opengl.GLUtils;
import android.os.Handler;
import android.text.InputType;
import android.text.method.PasswordTransformationMethod;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.widget.EditText;
import android.widget.Toast;
import android.widget.ImageView.ScaleType;


public class VncCanvas extends GLSurfaceView {
	private final static String TAG = "VncCanvas";

	AbstractScaling scaling;


	// Runtime control flags
	private AtomicBoolean showDesktopInfo = new AtomicBoolean(true);
	private boolean repaintsEnabled = true;

	/**
	 * Use camera button as meta key for right mouse button
	 */
	boolean cameraButtonDown = false;

	public VncCanvasActivity activity;

	// VNC protocol connection
	public VNCConn vncConn;

	public Handler handler = new Handler();

	private AbstractGestureInputHandler inputHandler;

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
	int mouseX, mouseY;

	/**
	 * Position of the top left portion of the <i>visible</i> part of the screen, in
	 * full-frame coordinates
	 */
	int absoluteXPosition = 0, absoluteYPosition = 0;




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

				if(vncConn.getFramebuffer() instanceof LargeBitmapData) {

					vncConn.lockFramebuffer();

					// Build Texture from bitmap
					GLUtils.texImage2D(GL10.GL_TEXTURE_2D, 0, vncConn.getFramebuffer().mbitmap, 0);

					vncConn.unlockFramebuffer();

				}

				if(vncConn.getFramebuffer() instanceof FullBufferBitmapData) {

					vncConn.lockFramebuffer();

					// build texture from pixel array
					gl.glTexImage2D(GL10.GL_TEXTURE_2D, 0, GL10.GL_RGBA,
							vncConn.getFramebuffer().bitmapwidth, vncConn.getFramebuffer().bitmapheight,
							0, GL10.GL_RGBA, GL10.GL_UNSIGNED_BYTE, IntBuffer.wrap(vncConn.getFramebuffer().bitmapPixels));

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
	void initializeVncCanvas(VncCanvasActivity a, AbstractGestureInputHandler inputHandler, VNCConn conn) {
		activity = a;
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
		mouseX=x;
		mouseY=y;
		vncConn.sendPointerEvent(x, y, 0, VNCConn.MOUSE_BUTTON_NONE);
	}


	private void mouseFollowPan()
	{
		try {
			if (vncConn.getConnSettings().getFollowPan() & scaling.isAbleToPan())
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
	 * Change to Canvas's scroll position to match the absoluteXPosition
	 */
	void scrollToAbsolute()
	{
		if(Utils.DEBUG()) Log.d(TAG, "scrollToAbsolute() " + absoluteXPosition + ", " + absoluteYPosition);
		float scale = getScale();
		scrollTo((int)((absoluteXPosition + ((float)getWidth() - vncConn.getFramebufferWidth()) / 2 ) * scale),
				(int)((absoluteYPosition + ((float)getHeight() - vncConn.getFramebufferHeight()) / 2 ) * scale));
	}

	/**
	 * Make sure mouse is visible on displayable part of screen
	 */
	void panToMouse()
	{
		if (! vncConn.getConnSettings().getFollowMouse())
			return;

		if (scaling != null && ! scaling.isAbleToPan())
			return;


		int x = mouseX;
		int y = mouseY;
		boolean panned = false;
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
			panned = true;
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
			panned = true;
		}
		if (panned)
		{
			scrollToAbsolute();
		}
	}

	/**
	 * Pan by a number of pixels (relative pan)
	 * @param dX
	 * @param dY
	 * @return True if the pan changed the view (did not move view out of bounds); false otherwise
	 */
	boolean pan(int dX, int dY) {

		double scale = getScale();

		double sX = (double)dX / scale;
		double sY = (double)dY / scale;

		if (absoluteXPosition + sX < 0)
			// dX = diff to 0
			sX = -absoluteXPosition;
		if (absoluteYPosition + sY < 0)
			sY = -absoluteYPosition;

		// Prevent panning right or below desktop image
		if (absoluteXPosition + getVisibleWidth() + sX > vncConn.getFramebufferWidth())
			sX = vncConn.getFramebufferWidth() - getVisibleWidth() - absoluteXPosition;
		if (absoluteYPosition + getVisibleHeight() + sY >  vncConn.getFramebufferHeight())
			sY = vncConn.getFramebufferHeight() - getVisibleHeight() - absoluteYPosition;

		absoluteXPosition += sX;
		absoluteYPosition += sY;

		// whene the frame buffer is smaller than the view,
		// it is is centered!
		if(vncConn.getFramebufferWidth() < getVisibleWidth())
			absoluteXPosition /= 2;
		if(vncConn.getFramebufferHeight() < getVisibleHeight())
			absoluteYPosition /= 2;


		if (sX != 0.0 || sY != 0.0)
		{
			scrollToAbsolute();
			return true;
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
			vncConn.getFramebuffer().scrollChanged(absoluteXPosition, absoluteYPosition);
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

	void reDraw() {

		if (repaintsEnabled && vncConn.getFramebuffer() != null) {

			// request a redraw from GL thread
			requestRender();

			// Show a Toast with the desktop info on first frame draw.
			if (showDesktopInfo.get()) {
				showDesktopInfo.set(false);

				handler.post(new Runnable() {
					@Override
					public void run() {
						showConnectionInfo();
					}
				});
			}

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
		String msg = vncConn.getDesktopName();
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
		try {
			if (enc != null && !enc.equals(""))
				msg += ", " + vncConn.getEncoding() + " encoding, " + vncConn.getColorModel().toString();
			else
				msg += ", " + vncConn.getColorModel().toString();
		}
		catch(NullPointerException e) {
		}
		Toast.makeText(getContext(), msg, Toast.LENGTH_LONG).show();
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
	 * @param downEvent True if "mouse button" (touch or trackball button) is down when this happens
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
		if (meta.isMouseClick())
		{
			vncConn.sendPointerEvent(mouseX, mouseY, meta.getMetaFlags(), meta.getMouseButtons());
			vncConn.sendPointerEvent(mouseX, mouseY, meta.getMetaFlags(), 0);
		}
		else {
			// KeyEvent(downTime, eventTime, action, code, repeat, metaState)
			KeyEvent downEvent = new KeyEvent(
					System.currentTimeMillis(),
					System.currentTimeMillis(),
					KeyEvent.ACTION_DOWN,
					meta.getKeySym(),
					0,
					meta.getMetaFlags());
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

	public int getVisibleWidth() {
		return (int)((double)getWidth() / getScale() + 0.5);
	}

	public int getVisibleHeight() {
		return (int)((double)getHeight() / getScale() + 0.5);
	}


	public int getCenteredXOffset() {
		int xoffset = (vncConn.getFramebufferWidth() - getWidth()) / 2;
		return xoffset;
	}

	public int getCenteredYOffset() {
		int yoffset = (vncConn.getFramebufferHeight() - getHeight()) / 2;
		return yoffset;
	}


	public void getCredFromUser(final ConnectionBean c) {
		// this method is probably called from the vnc thread
		post(new Runnable() {
			@Override
			public void run() {
				final EditText input = new EditText(getContext());
				input.setInputType(InputType.TYPE_TEXT_VARIATION_PASSWORD);
				input.setTransformationMethod(new PasswordTransformationMethod());

				new AlertDialog.Builder(getContext())
			    .setTitle(getContext().getString(R.string.password_caption))
			    .setView(input)
			    .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
			        public void onClick(DialogInterface dialog, int whichButton) {
			            c.setPassword(input.getText().toString());
			            synchronized (vncConn) {
				            vncConn.notify();
						}
			        }
			    }).show();
			}
		});

	}



	public ScaleType getScaleType() {
		// TODO Auto-generated method stub
		return null;
	}

	// called by zoomer
	public void setImageMatrix(Matrix matrix) {
		reDraw();
	}

	public void setScaleType(ScaleType scaleType) {
		// TODO Auto-generated method stub

	}
}
