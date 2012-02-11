/*
	Modified MTView from Multitouch Visible Test by Battery Powered Games LLC.
	Code is from http://www.rbgrn.net/content/367-source-code-to-multitouch-visible-test.
*/
package com.coboltforge.dontmind.multivnc;

import android.content.Context;
import android.graphics.BlurMaskFilter;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.BlurMaskFilter.Blur;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;


public class TouchPointView extends SurfaceView implements SurfaceHolder.Callback {
	@SuppressWarnings("unused")
	private static final String TAG = "TouchPointView";

	private static final int MAX_TOUCHPOINTS = 10;

	private Paint touchPaints[] = new Paint[MAX_TOUCHPOINTS];
	private int colors[] = new int[MAX_TOUCHPOINTS];

	private float scale = 1.0f;

	public TouchPointView(final Context context, AttributeSet attrs)
	{
		super(context, attrs);
		SurfaceHolder holder = getHolder();
		holder.addCallback(this);
		setFocusable(true); // make sure we get key events
		setFocusableInTouchMode(true); // make sure we get touch events

		colors[0] = Color.RED;
		colors[1] = Color.WHITE;
		colors[2] = Color.GREEN;
		colors[3] = Color.YELLOW;
		colors[4] = Color.CYAN;
		colors[5] = Color.MAGENTA;
		colors[6] = Color.DKGRAY;
		colors[7] = Color.BLUE;
		colors[8] = Color.LTGRAY;
		colors[9] = Color.GRAY;
		for (int i = 0; i < MAX_TOUCHPOINTS; i++) {
			touchPaints[i] = new Paint();
			touchPaints[i].setColor(colors[i]);
			touchPaints[i].setAlpha(200);
			touchPaints[i].setMaskFilter(new BlurMaskFilter(15, Blur.NORMAL));
		}
	}


	public boolean handleEvent(final MotionEvent event) {
		int pointerCount = event.getPointerCount();
		if (pointerCount > MAX_TOUCHPOINTS) {
			pointerCount = MAX_TOUCHPOINTS;
		}
		Canvas c = getHolder().lockCanvas();
		if (c != null) {
			c.drawColor(Color.BLACK);
			if (event.getAction() == MotionEvent.ACTION_UP) {
				// clear everything
			} else {
				for (int i = 0; i < pointerCount; i++) {
					int id = event.getPointerId(i);
					int x = (int) event.getX(i);
					int y = (int) event.getY(i);
					drawCircle(x, y, touchPaints[id], c);
				}
			}
			getHolder().unlockCanvasAndPost(c);
		}
		return true;
	}

	
	private void drawCircle(int x, int y, Paint paint, Canvas c) {
		c.drawCircle(x, y, 40 * scale, paint);
	}

	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
		if (width > height) {
			this.scale = width / 480f;
		} else {
			this.scale = height / 480f;
		}
		Canvas c = getHolder().lockCanvas();
		if (c != null) {
			// clear screen
			c.drawColor(Color.BLACK);
			getHolder().unlockCanvasAndPost(c);
		}
	}

	public void surfaceCreated(SurfaceHolder holder) {
	}

	public void surfaceDestroyed(SurfaceHolder holder) {
	}

}