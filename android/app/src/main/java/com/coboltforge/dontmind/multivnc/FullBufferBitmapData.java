/**
 * Copyright (c) 2010 Michael A. MacDonald
 */
package com.coboltforge.dontmind.multivnc;

import java.io.IOException;
import java.util.Arrays;

import android.graphics.Paint;
import android.graphics.Rect;

/**
 * @author Michael A. MacDonald
 *
 */
class FullBufferBitmapData extends AbstractBitmapData {

	int xoffset;
	int yoffset;
	
	/**
	 * Multiply this times total number of pixels to get estimate of process size with all buffers plus
	 * safety factor
	 */
	static final int CAPACITY_MULTIPLIER = 7;
	
	/**
	 * @param p
	 * @param c
	 */
	public FullBufferBitmapData(RfbProto p, VncCanvas c, int capacity) {
		super(p, c);
		framebufferwidth=rfb.framebufferWidth;
		framebufferheight=rfb.framebufferHeight;
		bitmapwidth= Utils.nextPow2(framebufferwidth);
		bitmapheight=Utils.nextPow2(framebufferheight);
		android.util.Log.i("FBBM", "bitmapsize = ("+bitmapwidth+","+bitmapheight+")");
		bitmapPixels = new int[bitmapwidth * bitmapheight];
	}

	/* (non-Javadoc)
	 * @see com.coboltforge.dontmind.multivnc.AbstractBitmapData#copyRect(android.graphics.Rect, android.graphics.Rect, android.graphics.Paint)
	 */
	@Override
	void copyRect(Rect src, Rect dest, Paint paint) {
		// TODO copy rect working?
		throw new RuntimeException( "copyrect Not implemented");
	}



	/* (non-Javadoc)
	 * @see com.coboltforge.dontmind.multivnc.AbstractBitmapData#drawRect(int, int, int, int, android.graphics.Paint)
	 */
	@Override
	synchronized void drawRect(int x, int y, int w, int h, Paint paint) {
		int color = paint.getColor();
		
		int offset = offset(x,y);
		if (w > 10)
		{
			for (int j = 0; j < h; j++, offset += bitmapwidth)
			{
				Arrays.fill(bitmapPixels, offset, offset + w, color);
			}
		}
		else
		{
			for (int j = 0; j < h; j++, offset += bitmapwidth - w)
			{
				for (int k = 0; k < w; k++, offset++)
				{
					bitmapPixels[offset] = color;
				}
			}
		}
	}

	/* (non-Javadoc)
	 * @see com.coboltforge.dontmind.multivnc.AbstractBitmapData#offset(int, int)
	 */
	@Override
	int offset(int x, int y) {
		return x + y * bitmapwidth;
	}

	/* (non-Javadoc)
	 * @see com.coboltforge.dontmind.multivnc.AbstractBitmapData#scrollChanged(int, int)
	 */
	@Override
	void scrollChanged(int newx, int newy) {
		xoffset = newx;
		yoffset = newy;
	}

	/* (non-Javadoc)
	 * @see com.coboltforge.dontmind.multivnc.AbstractBitmapData#syncScroll()
	 */
	@Override
	void syncScroll() {
		// Don't need to do anything here

	}

	/* (non-Javadoc)
	 * @see com.coboltforge.dontmind.multivnc.AbstractBitmapData#updateBitmap(int, int, int, int)
	 */
	@Override
	void updateBitmap(int x, int y, int w, int h) {
		// Don't need to do anything here

	}

	/* (non-Javadoc)
	 * @see com.coboltforge.dontmind.multivnc.AbstractBitmapData#validDraw(int, int, int, int)
	 */
	@Override
	boolean validDraw(int x, int y, int w, int h) {
		return true;
	}

	/* (non-Javadoc)
	 * @see com.coboltforge.dontmind.multivnc.AbstractBitmapData#writeFullUpdateRequest(boolean)
	 */
	@Override
	void writeFullUpdateRequest(boolean incremental) throws IOException {
		rfb.writeFramebufferUpdateRequest(0, 0, framebufferwidth, framebufferheight, incremental);
	}

}
