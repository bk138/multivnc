/**
 * Copyright (C) 2009 Michael A. MacDonald
 */
package com.coboltforge.dontmind.multivnc;

/**
 * @author Michael A. MacDonald
 */
class ZoomScaling {
	
	static final String TAG = "ZoomScaling";

	float currentScale = 1;
	float minimumScale = 1;
	float maximumScale = 4;

	/**
	 * Updates scale to given value.
	 * @param activity
	 */
	private void updateScale(float newScale, VncCanvasActivity activity) {
		//Clamp scale to min/max limits
		currentScale = Math.max(minimumScale, Math.min(newScale, maximumScale));

		//Update Zoom controls
		activity.zoomer.setIsZoomInEnabled(currentScale < maximumScale);
		activity.zoomer.setIsZoomOutEnabled(currentScale > minimumScale);

		//Refresh Canvas
		activity.vncCanvas.reDraw();
		activity.vncCanvas.scrollToAbsolute();
		activity.vncCanvas.pan(0, 0);

		//Notify user
		activity.showZoomLevel();
	}
	
	void zoomIn(VncCanvasActivity activity) {
		updateScale(currentScale + 0.25f, activity);
	}

	float getScale() {
		return currentScale;
	}

	void zoomOut(VncCanvasActivity activity) {
		updateScale(currentScale - 0.25f, activity);
	}

	void adjust(VncCanvasActivity activity, float scaleFactor, float fx, float fy) {
		updateScale(currentScale * scaleFactor, activity);

		//Keep the focal point fixed.
		int focusShiftX = (int) (fx * (1 - scaleFactor));
		int focusShiftY = (int) (fy * (1 - scaleFactor));
		activity.vncCanvas.pan(-focusShiftX, -focusShiftY);
	}

	/**
	 * Sets the activity's scale type to the scaling
	 * @param activity
	 */
	void setScaleTypeForActivity(VncCanvasActivity activity) {
		try {
			activity.zoomer.hide();
			activity.vncCanvas.scaling = this;
			currentScale = (float) 1.0;
			minimumScale = activity.vncCanvas.vncConn.getFramebuffer().getMinimumScale();
			// Reset the pan position to (0,0)
			updateScale(currentScale, activity);
		}
		catch(NullPointerException e) {
		}
	}
}
