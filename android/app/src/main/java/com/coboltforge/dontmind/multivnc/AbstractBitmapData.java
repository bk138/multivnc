/**
 * Copyright (C) 2009 Michael A. MacDonald
 */
package com.coboltforge.dontmind.multivnc;

import java.io.IOException;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.util.Log;

/**
 * Abstract interface between the VncCanvas and the bitmap and pixel data buffers that actually contain
 * the data.
 * This allows for implementations that use smaller bitmaps or buffers to save memory. 
 * @author Michael A. MacDonald
 *
 */
abstract class AbstractBitmapData {
	int framebufferwidth;
	int framebufferheight;
	int bitmapwidth;
	int bitmapheight;
	RfbProto rfb;
	Bitmap mbitmap;
	int bitmapPixels[];
	Canvas memGraphics;
	boolean waitingForInput;
	VncCanvas vncCanvas;

	AbstractBitmapData( RfbProto p, VncCanvas c)
	{
		rfb=p;
		vncCanvas = c;
		framebufferwidth=rfb.framebufferWidth;
		framebufferheight=rfb.framebufferHeight;
	}
	
	synchronized void doneWaiting()
	{
		waitingForInput=false;
	}
	
	final void invalidateMousePosition()
	{
		if (vncCanvas.vncConn.getConnSettings().getUseLocalCursor())
			// OpenGL always draws the full framebuffer, request a redraw from GL thread
			vncCanvas.requestRender();
	}
	
	/**
	 * 
	 * @return The smallest scale supported by the implementation; the scale at which
	 * the bitmap would be smaller than the screen
	 */
	float getMinimumScale()
	{
		double scale = 0.75;
		int displayWidth = vncCanvas.getWidth();
		int displayHeight = vncCanvas.getHeight();
		for (; scale >= 0; scale -= 0.25)
		{
			if (scale * bitmapwidth < displayWidth || scale * bitmapheight < displayHeight)
				break;
		}
		return (float)(scale + 0.25);
	}
	
	/**
	 * Send a request through the protocol to get the data for the currently held bitmap
	 * @param incremental True if we want incremental update; false for full update
	 */
	abstract void writeFullUpdateRequest( boolean incremental) throws IOException;
	
	/**
	 * Determine if a rectangle in full-frame coordinates can be drawn in the existing buffer
	 * @param x Top left x
	 * @param y Top left y
	 * @param w width (pixels)
	 * @param h height (pixels)
	 * @return True if entire rectangle fits into current screen buffer, false otherwise
	 */
	abstract boolean validDraw( int x, int y, int w, int h);
	
	/**
	 * Return an offset in the bitmapPixels array of a point in full-frame coordinates
	 * @param x
	 * @param y
	 * @return Offset in bitmapPixels array of color data for that point
	 */
	abstract int offset( int x, int y);
	
	/**
	 * Update pixels in the bitmap with data from the bitmapPixels array, positioned
	 * in full-frame coordinates
	 * @param x Top left x
	 * @param y Top left y
	 * @param w width (pixels)
	 * @param h height (pixels)
	 */
	abstract void updateBitmap( int x, int y, int w, int h);
	
	
	
	/**
	 * Copy a rectangle from one part of the bitmap to another
	 * @param src Rectangle in full-frame coordinates to be copied
	 * @param dest Destination rectangle in full-frame coordinates
	 * @param paint Paint specifier
	 */
	abstract void copyRect( Rect src, Rect dest, Paint paint);
	
	/**
	 * Draw a rectangle in the bitmap with coordinates given in full frame
	 * @param x Top left x
	 * @param y Top left y
	 * @param w width (pixels)
	 * @param h height (pixels)
	 * @param paint How to draw
	 */
	abstract void drawRect( int x, int y, int w, int h, Paint paint);
	
	/**
	 * Scroll position has changed.
	 * <p>
	 * This method is called in the UI thread-- it updates internal status, but does
	 * not change the bitmap data or send a network request until syncScroll is called
	 * @param newx Position of left edge of visible part in full-frame coordinates
	 * @param newy Position of top edge of visible part in full-frame coordinates
	 */
	abstract void scrollChanged( int newx, int newy);
	
	/**
	 * Sync scroll -- called from network thread; copies scroll changes from UI to network state
	 */
	abstract void syncScroll();

	void clear()
	{
		// blacken the int array that belongs to our bitmap
		if(bitmapPixels != null)
			for(int i=0; i<bitmapPixels.length; ++i)
				bitmapPixels[i] = 0;
	}
	
	
	Bitmap getScreenShot() {
		return Bitmap.createBitmap(bitmapPixels, 0, bitmapwidth, bitmapwidth, bitmapheight, Bitmap.Config.RGB_565);
	}
	
	/**
	 * Release resources
	 */
	void dispose()
	{
		if ( mbitmap!=null )
			mbitmap.recycle();
		memGraphics = null;
		bitmapPixels = null;
	}
	
	protected void finalize() {
		if(Utils.DEBUG()) Log.d("AbstractBitmapData", this + " finalized!");
	}
}
