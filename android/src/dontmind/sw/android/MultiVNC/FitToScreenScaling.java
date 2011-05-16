/**
 * Copyright (C) 2009 Michael A. MacDonald
 */
package dontmind.sw.android.MultiVNC;

import android.widget.ImageView.ScaleType;

/**
 * @author Michael A. MacDonald
 */
class FitToScreenScaling extends AbstractScaling {

	/**
	 * @param id
	 * @param scaleType
	 */
	FitToScreenScaling() {
		super(R.id.itemFitToScreen, ScaleType.FIT_CENTER);
	}

	/* (non-Javadoc)
	 * @see dontmind.sw.android.MultiVNC.AbstractScaling#isAbleToPan()
	 */
	@Override
	boolean isAbleToPan() {
		return false;
	}

	/* (non-Javadoc)
	 * @see dontmind.sw.android.MultiVNC.AbstractScaling#isValidInputMode(int)
	 */
	@Override
	boolean isValidInputMode(int mode) {
		return mode == R.id.itemInputFitToScreen;
	}

	/* (non-Javadoc)
	 * @see dontmind.sw.android.MultiVNC.AbstractScaling#getDefaultHandlerId()
	 */
	@Override
	int getDefaultHandlerId() {
		return R.id.itemInputFitToScreen;
	}

	/* (non-Javadoc)
	 * @see dontmind.sw.android.MultiVNC.AbstractScaling#setCanvasScaleType(dontmind.sw.android.MultiVNC.VncCanvas)
	 */
	@Override
	void setScaleTypeForActivity(VncCanvasActivity activity) {
		super.setScaleTypeForActivity(activity);
		activity.vncCanvas.absoluteXPosition = activity.vncCanvas.absoluteYPosition = 0;
		activity.vncCanvas.scrollTo(0, 0);
	}

}
