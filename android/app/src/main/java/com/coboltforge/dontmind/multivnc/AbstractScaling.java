/**
 * Copyright (C) 2009 Michael A. MacDonald
 */
package com.coboltforge.dontmind.multivnc;

/**
 * @author Michael A. MacDonald
 * 
 * A scaling mode for the VncCanvas; based on ImageView.ScaleType
 */
abstract class AbstractScaling {
	float getScale() { return 1; }
	
	void zoomIn(VncCanvasActivity activity) {}
	void zoomOut(VncCanvasActivity activity) {}

	/**
	 * Sets the activity's scale type to the scaling
	 * @param activity
	 */
	void setScaleTypeForActivity(VncCanvasActivity activity){
		activity.zoomer.hide();
		activity.vncCanvas.scaling = this;
	}

	/**
	 * Change the scaling and focus dynamically, as from a detected scale gesture
	 * @param activity Activity containing to canvas to scale
	 * @param scaleFactor Factor by which to adjust scaling
	 * @param fx Focus X of center of scale change
	 * @param fy Focus Y of center of scale change
	 */
	void adjust(VncCanvasActivity activity, float scaleFactor, float fx, float fy) {}
}
