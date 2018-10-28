/*
	Modified MTView from Multitouch Visible Test by Battery Powered Games LLC.
	Code is from http://www.rbgrn.net/content/367-source-code-to-multitouch-visible-test.
	Copyright © 2011-2012 Christian Beier <dontmind@freeshell.org>
 */
package com.coboltforge.dontmind.multivnc;

import android.content.Context;
import android.graphics.BitmapFactory;
import android.graphics.BlurMaskFilter;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.BlurMaskFilter.Blur;
import android.graphics.Shader;
import android.graphics.drawable.BitmapDrawable;
import android.util.AttributeSet;
import android.util.Log;
import android.util.TypedValue;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;


public class TouchPointView extends SurfaceView implements SurfaceHolder.Callback {
	@SuppressWarnings("unused")
	private static final String TAG = "TouchPointView";

	private static final int MAX_TOUCHPOINTS = 10;

	private Paint touchPaints[] = new Paint[MAX_TOUCHPOINTS];
	private int colors[] = new int[MAX_TOUCHPOINTS];
	private float radius;

	private BitmapDrawable background;

	private AbstractGestureInputHandler inputHandler;

	public TouchPointView(final Context context, AttributeSet attrs)
	{
		super(context, attrs);
		SurfaceHolder holder = getHolder();
		holder.addCallback(this);
		setFocusable(true); // make sure we get key events
		setFocusableInTouchMode(true); // make sure we get touch events

		background = new BitmapDrawable(getResources(), BitmapFactory.decodeResource(getResources(), R.drawable.touchpad));
		background.setTileModeX(Shader.TileMode.REPEAT);
		background.setTileModeY(Shader.TileMode.REPEAT);

		// a 50 dip radius
		radius = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 50, getResources().getDisplayMetrics());

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
			touchPaints[i].setAlpha(150);
			touchPaints[i].setMaskFilter(new BlurMaskFilter(15, Blur.NORMAL));
		}
	}


	@Override
	public boolean onTouchEvent(MotionEvent event) {
		
		int pointerCount = event.getPointerCount();
		if (pointerCount > MAX_TOUCHPOINTS) {
			pointerCount = MAX_TOUCHPOINTS;
		}
		Canvas c = getHolder().lockCanvas();
		if (c != null) {

			background.draw(c);

			Log.d("BUBU", "b " + getTop());

			if (event.getAction() == MotionEvent.ACTION_UP) {
				// clear everything
			} else {
				for (int i = 0; i < pointerCount; i++) {
					int id = event.getPointerId(i);
					int x = (int) event.getX(i);
					int y = (int) event.getY(i);
					Log.d("BUBU", "y " + y);

					drawCircle(x, y, touchPaints[id], c);
				}
			}
			getHolder().unlockCanvasAndPost(c);
		}
		
		return inputHandler.onTouchEvent(event);
	}


	private void drawCircle(int x, int y, Paint paint, Canvas c) {
		c.drawCircle(x, y, radius, paint);
	}

	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

		Canvas c = getHolder().lockCanvas();
		if (c != null) {
			background.setBounds(0, 0, width , height);
			// clear screen
			background.draw(c);
			getHolder().unlockCanvasAndPost(c);
		}
	}

	public void surfaceCreated(SurfaceHolder holder) {
	}

	public void surfaceDestroyed(SurfaceHolder holder) {
	}


	public void setInputHandler(AbstractGestureInputHandler inputHandler) {
		this.inputHandler = inputHandler;
	}

}