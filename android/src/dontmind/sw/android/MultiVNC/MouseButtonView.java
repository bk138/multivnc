package dontmind.sw.android.MultiVNC;

import java.io.IOException;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.HapticFeedbackConstants;
import android.view.MotionEvent;
import android.view.SoundEffectConstants;
import android.view.View;


public class MouseButtonView extends View {

	private final static String TAG = "MouseButtonView";

	public MouseButtonView(Context context, AttributeSet attrs) {
		super(context, attrs);
		setSoundEffectsEnabled(true);
	}


	public boolean handleEvent(MotionEvent e, int buttonId, 
				AbstractInputHandler inputHandler, VncCanvas canvas)
	{
		final int action = e.getAction();

		/* 
		 * in case one pointer holds the button down and another one tries
		 * to move the mouse, we need to feed the input handler with a 
		 * synthetic motion event.
		 */
		final int action_masked = action & MotionEvent.ACTION_MASK;
		if(e.getPointerCount() > 1 	
				&& ((action_masked == MotionEvent.ACTION_MOVE)
						|| action_masked == MotionEvent.ACTION_POINTER_UP
						|| action_masked == MotionEvent.ACTION_POINTER_DOWN))
		{
			if(Utils.DEBUG()) 
			{
				Log.d(TAG, "Input: button " + buttonId + " second pointer");
				inspectEvent(e);
			}

			// calc button view origin
			final float origin_x = e.getRawX() - e.getX();
			final float origin_y = e.getRawY() - e.getY();

			if(Utils.DEBUG()) Log.d(TAG, "Input: button " + buttonId + " origin: " + origin_x + "," + origin_y);

			int new_action = action;
			switch (action_masked)
			{
			case MotionEvent.ACTION_MOVE:
				new_action = MotionEvent.ACTION_MOVE;
				break;
			case MotionEvent.ACTION_POINTER_UP:
				new_action = MotionEvent.ACTION_UP;
				break;
			case MotionEvent.ACTION_POINTER_DOWN:
				new_action = MotionEvent.ACTION_DOWN;
				break;
			}
			
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
						new_action,  // action
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
						new_action,  // action, will always be ACTION_MOVE here
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

		if(action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_UP)
		{
			if(Utils.DEBUG()) 
				Log.d(TAG, "Input: button " + buttonId + " action:" + action);
			
			// bzzt!	
			performHapticFeedback(HapticFeedbackConstants.LONG_PRESS,
					HapticFeedbackConstants.FLAG_IGNORE_GLOBAL_SETTING|HapticFeedbackConstants.FLAG_IGNORE_VIEW_SETTING);
			
			// beep!
			playSoundEffect(SoundEffectConstants.CLICK);
			
			int pointerMask = 0; // like up
			if(action == MotionEvent.ACTION_DOWN)
			{
				switch(buttonId)
				{
				case 1:
					pointerMask = 1;
					break;
				case 2:
					pointerMask = 2;
					break;
				case 3:		
					pointerMask = 4;
					break;
				}
			}

			try {
				canvas.rfb.writePointerEvent(canvas.mouseX, canvas.mouseY, e.getMetaState(), pointerMask);
			} catch (IOException ex) {
				ex.printStackTrace();
			}

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
		Log.d(TAG, "Input: now @ " + e.getEventTime());
		for (int p = 0; p < pointerCount; p++) {
			Log.d(TAG, "Input:  pointer:" +
					e.getPointerId(p)
					+ " x:" + e.getX(p)
					+ " y:" + e.getY(p)
					+ " action:" + e.getAction());
		}
	}
}