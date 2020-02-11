/**
 * Copyright (C) 2009 Michael A. MacDonald
 */
package com.coboltforge.dontmind.multivnc;

import android.graphics.Matrix;
import android.widget.ImageView.ScaleType;

/**
 * @author Michael A. MacDonald
 */
class ZoomScaling extends AbstractScaling {

    static final String TAG = "ZoomScaling";

    private Matrix matrix;
    int canvasXOffset;
    int canvasYOffset;
    float scaling;
    float minimumScale;

    /**
     * @param id
     * @param scaleType
     */
    public ZoomScaling() {
        super(AbstractScaling.zoomableId, ScaleType.MATRIX);
        matrix = new Matrix();
        scaling = 1;
    }

    /* (non-Javadoc)
     * @see com.coboltforge.dontmind.multivnc.AbstractScaling#isAbleToPan()
     */
    @Override
    boolean isAbleToPan() {
        return true;
    }

    /* (non-Javadoc)
     * @see com.coboltforge.dontmind.multivnc.AbstractScaling#isValidInputMode(int)
     */
    @Override
    boolean isValidInputMode(int mode) {
        return true;
    }

    /**
     * Call after scaling and matrix have been changed to resolve scrolling
     *
     * @param activity
     */
    private void resolveZoom(VncCanvasActivity activity) {
        activity.vncCanvas.scrollToAbsolute();
        activity.vncCanvas.pan(0, 0);
    }

    /* (non-Javadoc)
     * @see com.coboltforge.dontmind.multivnc.AbstractScaling#zoomIn(com.coboltforge.dontmind.multivnc.VncCanvasActivity)
     */
    @Override
    void zoomIn(VncCanvasActivity activity) {
        resetMatrix();
        standardizeScaling();
        scaling += 0.25;
        if (scaling > 4.0) {
            scaling = (float) 4.0;
            activity.zoomer.setIsZoomInEnabled(false);
        }
        activity.zoomer.setIsZoomOutEnabled(true);
        matrix.postScale(scaling, scaling);
        //Log.v(TAG,String.format("before set matrix scrollx = %d scrolly = %d", activity.vncCanvas.getScrollX(), activity.vncCanvas.getScrollY()));
        activity.vncCanvas.setImageMatrix(matrix);
        resolveZoom(activity);
        // show scale
        activity.showScaleToast();
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
        resetMatrix();
        standardizeScaling();
        scaling -= 0.25;
        if (scaling < minimumScale) {
            scaling = minimumScale;
            activity.zoomer.setIsZoomOutEnabled(false);
        }
        activity.zoomer.setIsZoomInEnabled(true);
        matrix.postScale(scaling, scaling);
        //Log.v(TAG,String.format("before set matrix scrollx = %d scrolly = %d", activity.vncCanvas.getScrollX(), activity.vncCanvas.getScrollY()));
        activity.vncCanvas.setImageMatrix(matrix);
        //Log.v(TAG,String.format("after set matrix scrollx = %d scrolly = %d", activity.vncCanvas.getScrollX(), activity.vncCanvas.getScrollY()));
        resolveZoom(activity);
        // show scale
        activity.showScaleToast();
    }

    /* (non-Javadoc)
     * @see com.coboltforge.dontmind.multivnc.AbstractScaling#adjust(com.coboltforge.dontmind.multivnc.VncCanvasActivity, float, float, float)
     */
    @Override
    void adjust(VncCanvasActivity activity, float scaleFactor, float fx,
                float fy) {
        float newScale = scaleFactor * scaling;
        if (scaleFactor < 1) {
            if (newScale < minimumScale) {
                newScale = minimumScale;
                activity.zoomer.setIsZoomOutEnabled(false);
            }
            activity.zoomer.setIsZoomInEnabled(true);
        } else {
            if (newScale > 4) {
                newScale = 4;
                activity.zoomer.setIsZoomInEnabled(false);
            }
            activity.zoomer.setIsZoomOutEnabled(true);
        }
        // ax is the absolute x of the focus
        int xPan = activity.vncCanvas.absoluteXPosition;
        float ax = (fx / scaling) + xPan;
        float newXPan = (scaling * xPan - scaling * ax + newScale * ax) / newScale;
        int yPan = activity.vncCanvas.absoluteYPosition;
        float ay = (fy / scaling) + yPan;
        float newYPan = (scaling * yPan - scaling * ay + newScale * ay) / newScale;
        resetMatrix();
        scaling = newScale;
        matrix.postScale(scaling, scaling);
        activity.vncCanvas.setImageMatrix(matrix);
        resolveZoom(activity);
        activity.vncCanvas.pan((int) (newXPan - xPan), (int) (newYPan - yPan));
    }

    private void resetMatrix() {
        matrix.reset();
        matrix.preTranslate(canvasXOffset, canvasYOffset);
    }

    /**
     * Set scaling to one of the clicks on the zoom scale
     */
    private void standardizeScaling() {
        scaling = ((float) ((int) (scaling * 4))) / 4;
    }

    /* (non-Javadoc)
     * @see com.coboltforge.dontmind.multivnc.AbstractScaling#setScaleTypeForActivity(com.coboltforge.dontmind.multivnc.VncCanvasActivity)
     */
    @Override
    void setScaleTypeForActivity(VncCanvasActivity activity) {
        try {
            super.setScaleTypeForActivity(activity);
            scaling = (float) 1.0;
            minimumScale = activity.vncCanvas.vncConn.getFramebuffer().getMinimumScale();
            canvasXOffset = -activity.vncCanvas.getCenteredXOffset();
            canvasYOffset = -activity.vncCanvas.getCenteredYOffset();
            resetMatrix();
            activity.vncCanvas.setImageMatrix(matrix);
            // Reset the pan position to (0,0)
            resolveZoom(activity);
        } catch (NullPointerException e) {
        }
    }
}
