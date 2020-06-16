/**
 * Copyright (C) 2009 Michael A. MacDonald
 */
package com.coboltforge.dontmind.multivnc;

/**
 * @author Michael A. MacDonald
 */
class ZoomScaling extends AbstractScaling {
	
	static final String TAG = "ZoomScaling";

	float scaling = 1;
	float minimumScale;

	/**
	 * Call after scaling and matrix have been changed to resolve scrolling
	 * @param activity
	 */
	private void resolveZoom(VncCanvasActivity activity) {
		activity.vncCanvas.scrollToAbsolute();
		activity.vncCanvas.pan(0,0);
	}
	
	/* (non-Javadoc)
	 * @see com.coboltforge.dontmind.multivnc.AbstractScaling#zoomIn(com.coboltforge.dontmind.multivnc.VncCanvasActivity)
	 */
	@Override
	void zoomIn(VncCanvasActivity activity) {
		standardizeScaling();
		scaling += 0.25;
		if (scaling > 4.0)
		{
			scaling = (float)4.0;
			activity.zoomer.setIsZoomInEnabled(false);
		}
		activity.zoomer.setIsZoomOutEnabled(true);
		//Log.v(TAG,String.format("before set matrix scrollx = %d scrolly = %d", activity.vncCanvas.getScrollX(), activity.vncCanvas.getScrollY()));
		activity.vncCanvas.reDraw();
		resolveZoom(activity);
		activity.showZoomLevel();
	}

	/* (non-Javadoc)
	 * @see com.coboltforge.dontmind.multivnc.AbstractScaling#getScale()
	 */
	@Override
	float getScale() {
		return scaling;
	}

	/* (non-Javadoc)
	 * @see com.coboltforge.dontmind.multivnc.AbstractScaling#zoomOut(com.coboltforge.dontmind.multivnc.VncCanvasActivity)
	 */
	@Override
	void zoomOut(VncCanvasActivity activity) {
		standardizeScaling();
		scaling -= 0.25;
		if (scaling < minimumScale)
		{
			scaling = minimumScale;
			activity.zoomer.setIsZoomOutEnabled(false);
		}
		activity.zoomer.setIsZoomInEnabled(true);
		//Log.v(TAG,String.format("before set matrix scrollx = %d scrolly = %d", activity.vncCanvas.getScrollX(), activity.vncCanvas.getScrollY()));
		activity.vncCanvas.reDraw();
		//Log.v(TAG,String.format("after set matrix scrollx = %d scrolly = %d", activity.vncCanvas.getScrollX(), activity.vncCanvas.getScrollY()));
		resolveZoom(activity);
		activity.showZoomLevel();
	}

	/* (non-Javadoc)
	 * @see com.coboltforge.dontmind.multivnc.AbstractScaling#adjust(com.coboltforge.dontmind.multivnc.VncCanvasActivity, float, float, float)
	 */
	@Override
	void adjust(VncCanvasActivity activity, float scaleFactor, float fx, float fy) {
		float newScale = scaleFactor * scaling;
		if (scaleFactor < 1)
		{
			if (newScale < minimumScale)
			{
				newScale = minimumScale;
				activity.zoomer.setIsZoomOutEnabled(false);
			}
			activity.zoomer.setIsZoomInEnabled(true);
		}
		else
		{
			if (newScale > 4)
			{
				newScale = 4;
				activity.zoomer.setIsZoomInEnabled(false);
			}
			activity.zoomer.setIsZoomOutEnabled(true);
		}
		scaling = newScale;
		resolveZoom(activity);

		//Keep the focal point fixed.
		int focusShiftX = (int) (fx * (1 - scaleFactor));
		int focusShiftY = (int) (fy * (1 - scaleFactor));
		activity.vncCanvas.pan(-focusShiftX, -focusShiftY);
		activity.showZoomLevel();
	}

	/**
	 *  Set scaling to one of the clicks on the zoom scale
	 */
	private void standardizeScaling()
	{
		scaling = ((float)((int)(scaling * 4))) / 4;
	}

	/* (non-Javadoc)
	 * @see com.coboltforge.dontmind.multivnc.AbstractScaling#setScaleTypeForActivity(com.coboltforge.dontmind.multivnc.VncCanvasActivity)
	 */
	@Override
	void setScaleTypeForActivity(VncCanvasActivity activity) {
		try {
			super.setScaleTypeForActivity(activity);
			scaling = (float)1.0;
			minimumScale = activity.vncCanvas.vncConn.getFramebuffer().getMinimumScale();
			activity.vncCanvas.reDraw();
			// Reset the pan position to (0,0)
			resolveZoom(activity);
		}
		catch(NullPointerException e) {
		}

	}

}
