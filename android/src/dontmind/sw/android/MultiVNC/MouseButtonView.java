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
	private float dragX, dragY;

	public MouseButtonView(Context context, AttributeSet attrs) {
		super(context, attrs);
		setSoundEffectsEnabled(true);
	}


	public boolean handleEvent(MotionEvent e, int buttonId, VncCanvas canvas)
	{
		final int action = e.getAction();

		/* 
		 * in case one pointer holds the button down and another one tries
		 * to move the mouse
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
			
			// from there, get second pointer's _absolute_ coords
			final float pointer_x = origin_x + e.getX(1); 
			final float pointer_y = origin_y + e.getY(1); 

			switch (action_masked)
			{
			case MotionEvent.ACTION_MOVE:
				
				if(Utils.DEBUG()) Log.d(TAG, "Input: button " + buttonId + " second pointer pos: " + pointer_x + "," + pointer_y);
				
				// compute the relative movement offset on the remote screen.
				float deltaX = (pointer_x - dragX) * canvas.getScale();
				float deltaY = (pointer_y - dragY) * canvas.getScale();
				dragX = pointer_x;
				dragY = pointer_y;
				deltaX = fineCtrlScale(deltaX);
				deltaY = fineCtrlScale(deltaY);
				
				// compute the absolute new mouse pos on the remote site.
				float newRemoteX = canvas.mouseX + deltaX;
				float newRemoteY = canvas.mouseY + deltaY;
								
				try {
					canvas.rfb.writePointerEvent((int)newRemoteX, (int)newRemoteY, e.getMetaState(), 1 << (buttonId-1));
				} catch (IOException ex) {
					ex.printStackTrace();
				}
				
				// update view
				// FIXME use only canvas, not rfb directly
				canvas.mouseX = (int) newRemoteX;
				canvas.mouseY = (int) newRemoteY;
				canvas.panToMouse();
				
				break;
			case MotionEvent.ACTION_POINTER_DOWN:
				if(Utils.DEBUG()) Log.d(TAG, "Input: button " + buttonId + " second pointer pos: " + pointer_x + "," + pointer_y + " DOWN");
				dragX = pointer_x;
				dragY = pointer_y;
				break;
			}
		}

		// one pointer
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
				pointerMask = 1 << (buttonId-1);

			try {
				canvas.rfb.writePointerEvent(canvas.mouseX, canvas.mouseY, e.getMetaState(), pointerMask);
			} catch (IOException ex) {
				ex.printStackTrace();
			}

		}

		// we have to return true to get the ACTION_UP event 
		return true;
	}

	/**
	 * scale down delta when it is small. This will allow finer control
	 * when user is making a small movement on touch screen.
	 * Scale up delta when delta is big. This allows fast mouse movement when
	 * user is flinging.
	 * @param deltaX
	 * @return
	 */
	private float fineCtrlScale(float delta) {
		float sign = (delta>0) ? 1 : -1;
		delta = Math.abs(delta);
		if (delta>=1 && delta <=3) {
			delta = 1;
		}else if (delta <= 10) {
			delta *= 0.34;
		} else if (delta <= 30 ) {
			delta *= delta/30;
		} else if (delta <= 90) {
			delta *=  (delta/30);
		} else {
			delta *= 3.0;
		}
		return sign * delta;
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