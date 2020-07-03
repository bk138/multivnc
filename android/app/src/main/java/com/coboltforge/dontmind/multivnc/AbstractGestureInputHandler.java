/**
 * Copyright (C) 2009 Michael A. MacDonald
 */
package com.coboltforge.dontmind.multivnc;

import android.opengl.GLSurfaceView;
import android.view.GestureDetector;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;

/**
 * An AbstractInputHandler that uses GestureDetector to detect standard gestures in touch events
 *
 * @author Michael A. MacDonald
 */
abstract class AbstractGestureInputHandler extends GestureDetector.SimpleOnGestureListener implements ScaleGestureDetector.OnScaleGestureListener {
	protected GestureDetector gestures;
	protected ScaleGestureDetector scaleGestures;
	private VncCanvasActivity activity;

	float xInitialFocus;
	float yInitialFocus;
	boolean inScaling;

	private static final String TAG = "AbstractGestureInputHandler";

	AbstractGestureInputHandler(VncCanvasActivity c)
	{
		activity = c;
		gestures= new GestureDetector(c, this, null, false); // this is a SDK 8+ feature and apparently needed if targetsdk is set
		gestures.setOnDoubleTapListener(this);
		scaleGestures = new ScaleGestureDetector(c, this);
		scaleGestures.setQuickScaleEnabled(false);
	}

	public boolean onTouchEvent(MotionEvent evt) {
		scaleGestures.onTouchEvent(evt);
		return gestures.onTouchEvent(evt);
	}

	abstract public boolean onGenericMotionEvent(MotionEvent event);

	@Override
	public boolean onScale(ScaleGestureDetector detector) {
		boolean consumed = true;
		float fx = detector.getFocusX();
		float fy = detector.getFocusY();

		if (Math.abs(1.0 - detector.getScaleFactor()) < 0.01)
			consumed = false;

		//`inScaling` is used to disable multi-finger scroll/fling gestures while scaling.
		//But instead of setting it in `onScaleBegin()`, we do it here after some checks.
		//This is a work around for some devices which triggers scaling very early which
		//disables fling gestures.
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

	@Override
	public boolean onScaleBegin(ScaleGestureDetector detector) {
		xInitialFocus = detector.getFocusX();
		yInitialFocus = detector.getFocusY();
		inScaling = false;
		// set to continuous drawing for smoother screen updates
		activity.vncCanvas.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
		//Log.i(TAG,"scale begin ("+xInitialFocus+","+yInitialFocus+")");
		return true;
	}

	@Override
	public void onScaleEnd(ScaleGestureDetector detector) {
		//Log.i(TAG,"scale end");
		inScaling = false;
		// reset to on-request drawing to save battery
		activity.vncCanvas.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
	}

	protected boolean isTouchEvent(MotionEvent event) {
		return event.getSource() == InputDevice.SOURCE_TOUCHSCREEN ||
				event.getSource() == InputDevice.SOURCE_TOUCHPAD;
	}
}
