/**
 * Copyright (C) 2009 Michael A. MacDonald
 */
package com.coboltforge.dontmind.multivnc;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.view.GestureDetector;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;

/**
 * An AbstractInputHandler that uses GestureDetector to detect standard gestures in touch events
 *
 * @author Michael A. MacDonald
 */
@SuppressLint("NewApi")
abstract class AbstractGestureInputHandler extends GestureDetector.SimpleOnGestureListener implements ScaleGestureDetector.OnScaleGestureListener {
	protected GestureDetector gestures;
	protected ScaleGestureDetector scaleGestures;
	private VncCanvasActivity activity;

	float xInitialFocus;
	float yInitialFocus;
	float scale_orig;
	boolean inScaling;

	private static final String TAG = "AbstractGestureInputHandler";

	@TargetApi(8)
	AbstractGestureInputHandler(VncCanvasActivity c)
	{
		activity = c;
		if(Build.VERSION.SDK_INT < 8)
			gestures  = new GestureDetector(c, this);
		else
			gestures= new GestureDetector(c, this, null, false); // this is a SDK 8+ feature and apparently needed if targetsdk is set
		gestures.setOnDoubleTapListener(this);
		scaleGestures = new ScaleGestureDetector(c, this);
	}

	public boolean onTouchEvent(MotionEvent evt) {
		scaleGestures.onTouchEvent(evt);
		return gestures.onTouchEvent(evt);
	}

	abstract public boolean onGenericMotionEvent(MotionEvent event);

	/* (non-Javadoc)
	 * @see com.antlersoft.android.bc.OnScaleGestureListener#onScale(com.antlersoft.android.bc.IBCScaleGestureDetector)
	 */
	@Override
	public boolean onScale(ScaleGestureDetector detector) {
		boolean consumed = true;
		float fx = detector.getFocusX();
		float fy = detector.getFocusY();

		if (Math.abs(1.0 - detector.getScaleFactor()) < 0.02)
			consumed = false;

		if (!inScaling) {
			double xfs = fx - xInitialFocus;
			double yfs = fy - yInitialFocus;
			double fs = Math.sqrt(xfs * xfs + yfs * yfs);
			if (fs * 2 < Math.abs(detector.getCurrentSpan() - detector.getPreviousSpan())) {
				inScaling = true;
			}
		}

		if (consumed && inScaling) {
			if (activity.vncCanvas != null && activity.vncCanvas.scaling != null)
				activity.vncCanvas.scaling.adjust(activity, detector.getScaleFactor(), fx, fy);
		}

		return consumed;
	}

	/* (non-Javadoc)
	 * @see com.antlersoft.android.bc.OnScaleGestureListener#onScaleBegin(com.antlersoft.android.bc.IBCScaleGestureDetector)
	 */
	@Override
	public boolean onScaleBegin(ScaleGestureDetector detector) {
		xInitialFocus = detector.getFocusX();
		yInitialFocus = detector.getFocusY();
		inScaling = false;
		scale_orig = activity.vncCanvas.getScale();
		// set to continuous drawing for smoother screen updates
		activity.vncCanvas.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
		//Log.i(TAG,"scale begin ("+xInitialFocus+","+yInitialFocus+")");
		return true;
	}

	/* (non-Javadoc)
	 * @see com.antlersoft.android.bc.OnScaleGestureListener#onScaleEnd(com.antlersoft.android.bc.IBCScaleGestureDetector)
	 */
	@Override
	public void onScaleEnd(ScaleGestureDetector detector) {
		//Log.i(TAG,"scale end");
		inScaling = false;
		// reset to on-request drawing to save battery
		activity.vncCanvas.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
		// show scale
		if( activity.vncCanvas.getScale() != scale_orig )
			activity.showScaleToast();
	}


	@TargetApi(9)
	protected boolean isTouchEvent(MotionEvent event) {
		if(Build.VERSION.SDK_INT >= 9) {
			if ((event.getSource() == InputDevice.SOURCE_TOUCHSCREEN) ||
				(event.getSource() == InputDevice.SOURCE_TOUCHPAD)) {
				return true;
			}
			else {
				return false;
			}
		}

		// Make sure older APIs get the event
		return true;
	}
}
