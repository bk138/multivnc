package com.coboltforge.dontmind.multivnc.ui;

import android.os.Build;
import android.util.Log;
import android.view.GestureDetector;
import android.view.HapticFeedbackConstants;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.SoundEffectConstants;
import android.view.VelocityTracker;
import android.view.View;

import com.coboltforge.dontmind.multivnc.Utils;
import com.coboltforge.dontmind.multivnc.VNCConn;
import com.coboltforge.dontmind.multivnc.db.MetaKeyBean;

/**
 * Handles pointer input from:
 *   * touchscreen
 *   * stylus
 *   * physical mouse
 * and uses this for:
 *   * scaling
 *   * two-finger fling
 *   * pointer movement
 *   * click
 *   * drag
 * detection and handover of events to VncCanvasActivity and VncCanvas.
 */
@SuppressWarnings("FieldCanBeLocal")
public class PointerInputHandler extends GestureDetector.SimpleOnGestureListener implements ScaleGestureDetector.OnScaleGestureListener {

    private static final String TAG = "PointerInputHandler";

    private final VncCanvasActivity vncCanvasActivity;
    protected GestureDetector gestures;
    protected ScaleGestureDetector scaleGestures;

    float xInitialFocus;
    float yInitialFocus;
    boolean inScaling;

    /*
     * Samsung S Pen support
     */
    private final int SAMSUNG_SPEN_ACTION_MOVE = 213;
    private final int SAMSUNG_SPEN_ACTION_UP = 212;
    private final int SAMSUNG_SPEN_ACTION_DOWN = 211;
    private final String SAMSUNG_MANUFACTURER_NAME = "samsung";


    /**
     * In drag mode (entered with long press) you process mouse events
     * without sending them through the gesture detector
     */
    private boolean dragMode;
    float dragX, dragY;
    private boolean dragModeButtonDown = false;
    private boolean dragModeButton2insteadof1 = false;

    /*
     * two-finger fling gesture stuff
     */
    private long twoFingerFlingStart = -1;
    private VelocityTracker twoFingerFlingVelocityTracker;
    private boolean twoFingerFlingDetected = false;
    private final int TWO_FINGER_FLING_UNITS = 1000;
    private final float TWO_FINGER_FLING_THRESHOLD = 1000;

    PointerInputHandler(VncCanvasActivity vncCanvasActivity) {
        this.vncCanvasActivity = vncCanvasActivity;
        gestures= new GestureDetector(vncCanvasActivity, this, null, false); // this is a SDK 8+ feature and apparently needed if targetsdk is set
        gestures.setOnDoubleTapListener(this);
        scaleGestures = new ScaleGestureDetector(vncCanvasActivity, this);
        scaleGestures.setQuickScaleEnabled(false);

        Log.d(TAG, "MightyInputHandler " + this +  " created!");
    }


    public void init() {
        twoFingerFlingVelocityTracker = VelocityTracker.obtain();
        Log.d(TAG, "MightyInputHandler " + this +  " init!");
    }

    public void shutdown() {
        try {
            twoFingerFlingVelocityTracker.recycle();
            twoFingerFlingVelocityTracker = null;
        }
        catch (NullPointerException ignored) {
        }
        Log.d(TAG, "MightyInputHandler " + this +  " shutdown!");
    }


    protected boolean isTouchEvent(MotionEvent event) {
        return event.getSource() == InputDevice.SOURCE_TOUCHSCREEN ||
                event.getSource() == InputDevice.SOURCE_TOUCHPAD;
    }


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
            if (vncCanvasActivity.vncCanvas != null && vncCanvasActivity.vncCanvas.scaling != null)
                vncCanvasActivity.vncCanvas.scaling.adjust(detector.getScaleFactor(), fx, fy);
        }

        return consumed;
    }

    @Override
    public boolean onScaleBegin(ScaleGestureDetector detector) {
        xInitialFocus = detector.getFocusX();
        yInitialFocus = detector.getFocusY();
        inScaling = false;
        //Log.i(TAG,"scale begin ("+xInitialFocus+","+yInitialFocus+")");
        return true;
    }

    @Override
    public void onScaleEnd(ScaleGestureDetector detector) {
        //Log.i(TAG,"scale end");
        inScaling = false;
    }

    /**
     * scale down delta when it is small. This will allow finer control
     * when user is making a small movement on touch screen.
     * Scale up delta when delta is big. This allows fast mouse movement when
     * user is flinging.
     */
    public static float fineCtrlScale(float delta) {
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

    private void twoFingerFlingNotification(String str)
    {
        // bzzt!
        vncCanvasActivity.vncCanvas.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY,
                HapticFeedbackConstants.FLAG_IGNORE_GLOBAL_SETTING|HapticFeedbackConstants.FLAG_IGNORE_VIEW_SETTING);

        // beep!
        vncCanvasActivity.vncCanvas.playSoundEffect(SoundEffectConstants.CLICK);

        vncCanvasActivity.notificationToast.setText(str);
        vncCanvasActivity.notificationToast.show();
    }

    private void twoFingerFlingAction(Character d)
    {
        switch(d) {
        case '←':
            vncCanvasActivity.vncCanvas.sendMetaKey(MetaKeyBean.keyArrowLeft);
            break;
        case '→':
            vncCanvasActivity.vncCanvas.sendMetaKey(MetaKeyBean.keyArrowRight);
            break;
        case '↑':
            vncCanvasActivity.vncCanvas.sendMetaKey(MetaKeyBean.keyArrowUp);
            break;
        case '↓':
            vncCanvasActivity.vncCanvas.sendMetaKey(MetaKeyBean.keyArrowDown);
            break;
        }
    }

    @Override
    public void onLongPress(MotionEvent e) {

        if (!isTouchEvent(e))
            return;

        if(Utils.DEBUG()) Log.d(TAG, "Input: long press");

        dragMode = true;
        dragX = e.getX();
        dragY = e.getY();

        // only interpret as button down if virtual mouse buttons are disabled
        if(vncCanvasActivity.mousebuttons.getVisibility() != View.VISIBLE) {
            dragModeButtonDown = true;
            vncCanvasActivity.vncCanvas.performHapticFeedback(HapticFeedbackConstants.LONG_PRESS,
                    HapticFeedbackConstants.FLAG_IGNORE_GLOBAL_SETTING|HapticFeedbackConstants.FLAG_IGNORE_VIEW_SETTING);
        }
    }

    @Override
    public boolean onScroll(MotionEvent e1, MotionEvent e2,
            float distanceX, float distanceY) {

        if (!isTouchEvent(e1))
            return false;

        if (!isTouchEvent(e2))
            return false;

        if (e2.getPointerCount() > 1)
        {
            if(Utils.DEBUG()) Log.d(TAG, "Input: scroll multitouch");

            if (inScaling)
                return false;

            // pan on 3 fingers and more
            if(e2.getPointerCount() > 2) {
                return vncCanvasActivity.vncCanvas.pan((int) distanceX, (int) distanceY);
            }

            /*
             * here comes the stuff that acts for two fingers
             */

            // gesture start
            if(twoFingerFlingStart != e1.getEventTime()) // it's a new scroll sequence
            {
                twoFingerFlingStart = e1.getEventTime();
                twoFingerFlingDetected = false;
                twoFingerFlingVelocityTracker.clear();
                if(Utils.DEBUG()) Log.d(TAG, "new twoFingerFling detection started");
            }

            // gesture end
            if(!twoFingerFlingDetected) // not yet detected in this sequence (we only want ONE event per sequence!)
            {
                // update our velocity tracker
                twoFingerFlingVelocityTracker.addMovement(e2);
                twoFingerFlingVelocityTracker.computeCurrentVelocity(TWO_FINGER_FLING_UNITS);

                float velocityX = twoFingerFlingVelocityTracker.getXVelocity();
                float velocityY = twoFingerFlingVelocityTracker.getYVelocity();

                // check for left/right flings
                if(Math.abs(velocityX) > TWO_FINGER_FLING_THRESHOLD
                        && Math.abs(velocityX) > Math.abs(2*velocityY)) {
                    if(velocityX < 0) {
                        if(Utils.DEBUG()) Log.d(TAG, "twoFingerFling LEFT detected");
                        twoFingerFlingNotification("←");
                        twoFingerFlingAction('←');
                    }
                    else {
                        if(Utils.DEBUG()) Log.d(TAG, "twoFingerFling RIGHT detected");
                        twoFingerFlingNotification("→");
                        twoFingerFlingAction('→');
                    }

                    twoFingerFlingDetected = true;
                }

                // check for left/right flings
                if(Math.abs(velocityY) > TWO_FINGER_FLING_THRESHOLD
                        && Math.abs(velocityY) > Math.abs(2*velocityX)) {
                    if(velocityY < 0) {
                        if(Utils.DEBUG()) Log.d(TAG, "twoFingerFling UP detected");
                        twoFingerFlingNotification("↑");
                        twoFingerFlingAction('↑');
                    }
                    else {
                        if(Utils.DEBUG()) Log.d(TAG, "twoFingerFling DOWN detected");
                        twoFingerFlingNotification("↓");
                        twoFingerFlingAction('↓');
                    }

                    twoFingerFlingDetected = true;
                }

            }

            return twoFingerFlingDetected;
        }
        else
        {
            // compute the relative movement offset on the remote screen.
            float deltaX = -distanceX;
            float deltaY = -distanceY;
            deltaX = fineCtrlScale(deltaX);
            deltaY = fineCtrlScale(deltaY);

            // compute the absolution new mouse pos on the remote site.
            float newRemoteX = vncCanvasActivity.vncCanvas.mouseX + deltaX;
            float newRemoteY = vncCanvasActivity.vncCanvas.mouseY + deltaY;

            if(Utils.DEBUG()) Log.d(TAG, "Input: scroll single touch from "
                    + vncCanvasActivity.vncCanvas.mouseX + "," + vncCanvasActivity.vncCanvas.mouseY
                    + " to " + (int)newRemoteX + "," + (int)newRemoteY);

//				if (dragMode) {
//
//					Log.d(TAG, "dragmode in scroll!!!!");
//
//					if (e2.getAction() == MotionEvent.ACTION_UP)
//						dragMode = false;
//					dragX = e2.getX();
//					dragY = e2.getY();
//					e2.setLocation(newRemoteX, newRemoteY);
//					return vncCanvas.processPointerEvent(e2, true);
//				}

            e2.setLocation(newRemoteX, newRemoteY);
            vncCanvasActivity.vncCanvas.processPointerEvent(e2, false);
        }
        return false;
    }


    public boolean onTouchEvent(MotionEvent e) {
        if (!isTouchEvent(e)) { // physical input device

            if(Utils.DEBUG()) Log.d(TAG, "Input: touch not screen nor pad: x:" + e.getX() + " y:" + e.getY() + " action:" + e.getAction());

            e = vncCanvasActivity.vncCanvas.changeTouchCoordinatesToFullFrame(e);

            // modify MotionEvent to support Samsung S Pen Event and activate rightButton accordingly
            // if Samsung S Pen is not present or reports false, check for right mouse button
            boolean isSPen = spenActionConvert(e);
            boolean isRightButton = e.isButtonPressed(MotionEvent.BUTTON_SECONDARY);

            if (isSPen || Build.VERSION.SDK_INT < 23)
                vncCanvasActivity.vncCanvas.processPointerEvent(e, true, isSPen || isRightButton);
            else if (e.getActionMasked() == MotionEvent.ACTION_MOVE)
                vncCanvasActivity.vncCanvas.processMouseEvent(0, true, (int) e.getX(), (int) e.getY());

            return true;
        }

        if (dragMode) {

            if(Utils.DEBUG()) Log.d(TAG, "Input: touch dragMode");

            // compute the relative movement offset on the remote screen.
            float deltaX = (e.getX() - dragX);
            float deltaY = (e.getY() - dragY);
            dragX = e.getX();
            dragY = e.getY();
            deltaX = fineCtrlScale(deltaX);
            deltaY = fineCtrlScale(deltaY);

            // compute the absolution new mouse pos on the remote site.
            float newRemoteX = vncCanvasActivity.vncCanvas.mouseX + deltaX;
            float newRemoteY = vncCanvasActivity.vncCanvas.mouseY + deltaY;


            if (e.getAction() == MotionEvent.ACTION_UP)
            {
                if(Utils.DEBUG()) Log.d(TAG, "Input: touch dragMode, finger up");

                dragMode = false;

                dragModeButtonDown = false;
                dragModeButton2insteadof1 = false;

                remoteMouseStayPut(e);
                vncCanvasActivity.vncCanvas.processPointerEvent(e, false);
                scaleGestures.onTouchEvent(e);
                return gestures.onTouchEvent(e); // important! otherwise the gesture detector gets confused!
            }
            e.setLocation(newRemoteX, newRemoteY);

            boolean status;
            if(dragModeButtonDown) {
                if(!dragModeButton2insteadof1) // button 1 down
                    status = vncCanvasActivity.vncCanvas.processPointerEvent(e, true, false);
                else // button2 down
                    status = vncCanvasActivity.vncCanvas.processPointerEvent(e, true, true);
            }
            else { // dragging without any button down
                status = vncCanvasActivity.vncCanvas.processPointerEvent(e, false);
            }

            return status;

        }


        if(Utils.DEBUG())
            Log.d(TAG, "Input: touch normal: x:" + e.getX() + " y:" + e.getY() + " action:" + e.getAction());

        scaleGestures.onTouchEvent(e);
        return gestures.onTouchEvent(e);
    }

    /**
     * Modify MotionEvent so that Samsung S Pen Actions are handled as normal Action; returns true when it's a Samsung S Pen Action
     */
    private boolean spenActionConvert(MotionEvent e) {
        if((e.getToolType(0) == MotionEvent.TOOL_TYPE_STYLUS) &&
                Build.MANUFACTURER.equals(SAMSUNG_MANUFACTURER_NAME)){

            switch(e.getAction()){
                case SAMSUNG_SPEN_ACTION_DOWN:
                    e.setAction(MotionEvent.ACTION_DOWN);
                    return true;
                case SAMSUNG_SPEN_ACTION_UP:
                    e.setAction(MotionEvent.ACTION_UP);
                    return true;
                case SAMSUNG_SPEN_ACTION_MOVE:
                    e.setAction(MotionEvent.ACTION_MOVE);
                    return true;
                default:
                    return false;
            }
        }
        return false;
    }

    public boolean onGenericMotionEvent(MotionEvent e) {
        if (Utils.DEBUG())
            Log.d(TAG, "Input: generic motion: " + e);

        if (isTouchEvent(e))
            return false;

        vncCanvasActivity.vncCanvas.changeTouchCoordinatesToFullFrame(e);
        int x = (int) e.getX();
        int y = (int) e.getY();

        switch (e.getAction()) {
            case MotionEvent.ACTION_HOVER_MOVE:
                vncCanvasActivity.vncCanvas.processMouseEvent(0, false, x, y);
                break;

            case MotionEvent.ACTION_SCROLL:
                if (e.getAxisValue(MotionEvent.AXIS_VSCROLL) < 0.0f) {
                    vncCanvasActivity.vncCanvas.processMouseEvent(VNCConn.MOUSE_BUTTON_SCROLL_DOWN, true, x, y);
                    vncCanvasActivity.vncCanvas.processMouseEvent(VNCConn.MOUSE_BUTTON_SCROLL_DOWN, false, x, y);
                } else {
                    vncCanvasActivity.vncCanvas.processMouseEvent(VNCConn.MOUSE_BUTTON_SCROLL_UP, true, x, y);
                    vncCanvasActivity.vncCanvas.processMouseEvent(VNCConn.MOUSE_BUTTON_SCROLL_UP, false, x, y);
                }
                break;
        }

        if (Build.VERSION.SDK_INT >= 23) {
            int button = androidButtonToVncButton(e.getActionButton());
            switch (e.getAction()) {
                case MotionEvent.ACTION_BUTTON_PRESS:
                    vncCanvasActivity.vncCanvas.processMouseEvent(button, true, x, y);
                    break;

                case MotionEvent.ACTION_BUTTON_RELEASE:
                    vncCanvasActivity.vncCanvas.processMouseEvent(button, false, x, y);
                    break;
            }
        }

        return true;
    }

    private int androidButtonToVncButton(int androidButton) {
        switch (androidButton) {
            case MotionEvent.BUTTON_PRIMARY:
                return VNCConn.MOUSE_BUTTON_LEFT;
            case MotionEvent.BUTTON_SECONDARY:
                return VNCConn.MOUSE_BUTTON_RIGHT;
            case MotionEvent.BUTTON_TERTIARY:
                return VNCConn.MOUSE_BUTTON_MIDDLE;
            default:
                return 0;
        }
    }

    /**
     * Modify the event so that it does not move the mouse on the
     * remote server.
     */
    private void remoteMouseStayPut(MotionEvent e) {
        e.setLocation(vncCanvasActivity.vncCanvas.mouseX, vncCanvasActivity.vncCanvas.mouseY);

    }

    /**
     * Do a left mouse down and up click on remote without moving the mouse.
     */
    @Override
    public boolean onSingleTapConfirmed(MotionEvent e) {

        if (!isTouchEvent(e))
            return false;

        // disable if virtual mouse buttons are in use
        if(vncCanvasActivity.mousebuttons.getVisibility()== View.VISIBLE)
            return false;

        if(Utils.DEBUG()) Log.d(TAG, "Input: single tap");

        boolean multiTouch = e.getPointerCount() > 1;
        remoteMouseStayPut(e);

        vncCanvasActivity.vncCanvas.processPointerEvent(e, true, multiTouch|| vncCanvasActivity.vncCanvas.cameraButtonDown);
        e.setAction(MotionEvent.ACTION_UP);
        return vncCanvasActivity.vncCanvas.processPointerEvent(e, false, multiTouch|| vncCanvasActivity.vncCanvas.cameraButtonDown);
    }

    /**
     * Do right mouse down and up click on remote without moving the mouse.
     */
    @Override
    public boolean onDoubleTap(MotionEvent e) {

        if (!isTouchEvent(e))
            return false;

        // disable if virtual mouse buttons are in use
        if(vncCanvasActivity.mousebuttons.getVisibility()== View.VISIBLE)
            return false;

        if(Utils.DEBUG()) Log.d(TAG, "Input: double tap");

        dragModeButtonDown = true;
        dragModeButton2insteadof1 = true;

        remoteMouseStayPut(e);
        vncCanvasActivity.vncCanvas.processPointerEvent(e, true, true);
        e.setAction(MotionEvent.ACTION_UP);
        return vncCanvasActivity.vncCanvas.processPointerEvent(e, false, true);
    }


    @Override
    public boolean onDown(MotionEvent e) {
        return isTouchEvent(e);
    }
}
