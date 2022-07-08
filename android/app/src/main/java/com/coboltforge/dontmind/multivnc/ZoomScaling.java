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
	VncCanvasActivity activity;

	ZoomScaling(VncCanvasActivity activity, float minimumScale, float maximumScale) {
		this.minimumScale = minimumScale;
		this.maximumScale = maximumScale;
		this.activity = activity;

		// Reset the pan position to (0,0)
		updateScale(currentScale);
	}

	/**
	 * Updates scale to given value.
	 */
	private void updateScale(float newScale) {
		float oldScale = currentScale;

		//Clamp scale to min/max limits
		currentScale = Math.max(minimumScale, Math.min(newScale, maximumScale));

		//Update Zoom controls
		activity.zoomer.setIsZoomInEnabled(currentScale < maximumScale);
		activity.zoomer.setIsZoomOutEnabled(currentScale > minimumScale);

		//Refresh Canvas
		activity.vncCanvas.reDraw();
		activity.vncCanvas.pan(0, 0);

		//Notify user
		if (newScale != oldScale)
			activity.showZoomLevel();
	}
	
	void zoomIn() {
		updateScale(currentScale + 0.25f);
	}

	float getScale() {
		return currentScale;
	}

	void zoomOut() {
		updateScale(currentScale - 0.25f);
	}

	void adjust(float scaleFactor, float fx, float fy) {
		updateScale(currentScale * scaleFactor);

		//Keep the focal point fixed.
		int focusShiftX = (int) (fx * (1 - scaleFactor));
		int focusShiftY = (int) (fy * (1 - scaleFactor));
		activity.vncCanvas.pan(-focusShiftX, -focusShiftY);
	}

}
