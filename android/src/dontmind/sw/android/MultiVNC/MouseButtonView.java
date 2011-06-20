package dontmind.sw.android.MultiVNC;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.HapticFeedbackConstants;
import android.view.MotionEvent;
import android.view.View;


public class MouseButtonView extends View {

	private final static String TAG = "MouseButtonView";

	public MouseButtonView(Context context, AttributeSet attrs) {
		super(context, attrs);

	}


	public boolean handleEvent(MotionEvent e, int buttonId, AbstractInputHandler inputHandler)
	{
		if(Utils.DEBUG()) 
			Log.d(TAG, "Input: button " + buttonId + " action:" + e.getAction());

		/* 
		 * in case one pointer holds the button down and another one tries
		 * to move the mouse, we need to feed the input handler with a 
		 * synthetic motion event.
		 */
		if(e.getPointerCount() > 1
				&& e.getAction() == MotionEvent.ACTION_MOVE)
		{
			if(Utils.DEBUG()) inspectEvent(e);

			// calc button view origin
			final float origin_x = e.getRawX() - e.getX();
			final float origin_y = e.getRawY() - e.getY();

			if(Utils.DEBUG()) Log.d(TAG, "Input button " + buttonId + " origin: " + origin_x + "," + origin_y);

			final int historySize = e.getHistorySize();

			// get a new MotionEvent.
			// we can not reuse the old one since there's now way to 
			// remove the second pointer from it.
			MotionEvent new_e;
			if(historySize == 0) // no history, simple case
			{
				new_e = MotionEvent.obtain(
						e.getDownTime(), // downtime //XXX?
						e.getEventTime(), // eventtime
						e.getAction(),  // action
						origin_x + e.getX(1), // x
						origin_y + e.getY(1), // y
						e.getPressure(1), // pressure
						e.getSize(1),  //size
						e.getMetaState(), // metastate
						e.getXPrecision(), e.getYPrecision(), // x,y precision
						e.getDeviceId(), // device id
						e.getEdgeFlags()); // edgeflags
			}
			else // there is history, bit more complicated
			{
				// get oldest one
				new_e = MotionEvent.obtain(
						e.getDownTime(), // downtime //XXX?
						e.getHistoricalEventTime(0), // oldest eventtime
						e.getAction(),  // action
						origin_x + e.getHistoricalX(1, 0), // oldest x
						origin_y + e.getHistoricalY(1, 0), // oldest y
						e.getHistoricalPressure(1, 0), // oldest pressure
						e.getHistoricalSize(1, 0),  // oldest size
						e.getMetaState(), // metastate
						e.getXPrecision(), e.getYPrecision(), // x,y precision
						e.getDeviceId(), // device id
						e.getEdgeFlags()); // edgeflags

				// add up more history. addBatch adds to front!
				for (int h = 1; h < historySize; ++h) // from 2nd oldest to 2nd newest
				{
					new_e.addBatch(
							e.getHistoricalEventTime(h),
							origin_x + e.getHistoricalX(1, h),
							origin_y + e.getHistoricalY(1, h),
							e.getHistoricalPressure(1, h), 
							e.getHistoricalSize(1, h),
							e.getMetaState());
				}

				// and the newest one
				new_e.addBatch(
						e.getEventTime(),
						origin_x + e.getX(1), // x
						origin_y + e.getY(1), // y
						e.getPressure(1), // pressure
						e.getSize(1),  //size
						e.getMetaState()); // metastate
			}

			if(Utils.DEBUG()) inspectEvent(new_e);

			// feed to input handler
			inputHandler.onTouchEvent(new_e);
		}
		else
		{
			// vibrate	
			performHapticFeedback(HapticFeedbackConstants.LONG_PRESS,
					HapticFeedbackConstants.FLAG_IGNORE_GLOBAL_SETTING|HapticFeedbackConstants.FLAG_IGNORE_VIEW_SETTING);

		}

		// we have to return true to get the ACTION_UP event 
		return true;
	}


	private void inspectEvent(MotionEvent e)
	{
		final int historySize = e.getHistorySize();
		final int pointerCount = e.getPointerCount();
		for (int h = 0; h < historySize; h++) {
			Log.d(TAG, "Input: historical @ " + e.getHistoricalEventTime(h));
			for (int p = 0; p < pointerCount; p++) {
				Log.d(TAG, "Input:  pointer:" +
						e.getPointerId(p) + " x:" +  e.getHistoricalX(p, h)
						+ " y:" +  e.getHistoricalY(p, h));
			}
		}
		for (int p = 0; p < pointerCount; p++) {
			Log.d(TAG, "Input: now @ " + e.getEventTime());
			Log.d(TAG, "Input:  pointer:" +
					e.getPointerId(p)
					+ " x:" + e.getX(p)
					+ " y:" + e.getY(p));
		}
	}
}