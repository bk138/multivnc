/**
 * Copyright (C) 2009 Michael A. MacDonald
 */
package com.coboltforge.dontmind.multivnc;

import android.annotation.TargetApi;
import android.os.Build;
import android.view.InputDevice;
import android.view.MotionEvent;

/**
 * An AbstractInputHandler that uses a mouse or stylus to detect motion events
 * 
 * @author Michael A. MacDonald
 */
abstract class AbstractPhysicalInputHandler {
	
	abstract public boolean onGenericMotionEvent(MotionEvent event);
	
	abstract public boolean onTouchEvent(MotionEvent event);

	@TargetApi(9)
	protected boolean isValidEvent(MotionEvent event) {
		if(Build.VERSION.SDK_INT >= 9) {
			if ((event.getSource() == InputDevice.SOURCE_TRACKBALL) || 
				(event.getSource() == InputDevice.SOURCE_MOUSE)) {
				
				if (event.getPointerCount() == 1) {
					return true;
				}
			}
		}
		return false;
	}
}
