/*
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

//
// CanvasView is the Activity for showing VNC Desktop.
//
package com.coboltforge.dontmind.multivnc;

import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.List;


import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.res.Configuration;
import android.app.ProgressDialog;
import android.os.Build;
import android.text.ClipboardManager;
import android.content.ContentValues;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.DialogInterface.OnDismissListener;
import android.database.Cursor;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Bundle;
import android.os.SystemClock;
import android.util.Log;
import android.view.GestureDetector;
import android.view.Gravity;
import android.view.HapticFeedbackConstants;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.SoundEffectConstants;
import android.view.VelocityTracker;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.PopupMenu;
import android.widget.TextView;
import android.widget.Toast;
import android.view.inputmethod.InputMethodManager;
import android.content.Context;

import com.google.android.material.floatingactionbutton.FloatingActionButton;

@SuppressWarnings("deprecation")
public class VncCanvasActivity extends Activity implements PopupMenu.OnMenuItemClickListener {


	public class MightyInputHandler extends GestureDetector.SimpleOnGestureListener implements ScaleGestureDetector.OnScaleGestureListener {

		private static final String TAG = "TouchPadInputHandler";

		protected GestureDetector gestures;
		protected ScaleGestureDetector scaleGestures;
		private VncCanvasActivity activity;

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

		MightyInputHandler() {
			activity = VncCanvasActivity.this;
			gestures= new GestureDetector(activity, this, null, false); // this is a SDK 8+ feature and apparently needed if targetsdk is set
			gestures.setOnDoubleTapListener(this);
			scaleGestures = new ScaleGestureDetector(activity, this);
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
			catch (NullPointerException e) {
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
				if (activity.vncCanvas != null && activity.vncCanvas.scaling != null)
					activity.vncCanvas.scaling.adjust(detector.getScaleFactor(), fx, fy);
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
		 * @param delta
		 * @return
		 */
		private float fineCtrlScale(float delta) {
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
			vncCanvas.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY,
					HapticFeedbackConstants.FLAG_IGNORE_GLOBAL_SETTING|HapticFeedbackConstants.FLAG_IGNORE_VIEW_SETTING);

			// beep!
			vncCanvas.playSoundEffect(SoundEffectConstants.CLICK);

			VncCanvasActivity.this.notificationToast.setText(str);
			VncCanvasActivity.this.notificationToast.show();
		}

		private void twoFingerFlingAction(Character d)
		{
			switch(d) {
			case '←':
				vncCanvas.sendMetaKey(MetaKeyBean.keyArrowLeft);
				break;
			case '→':
				vncCanvas.sendMetaKey(MetaKeyBean.keyArrowRight);
				break;
			case '↑':
				vncCanvas.sendMetaKey(MetaKeyBean.keyArrowUp);
				break;
			case '↓':
				vncCanvas.sendMetaKey(MetaKeyBean.keyArrowDown);
				break;
			}
		}


		/*
		 * (non-Javadoc)
		 *
		 * @see android.view.GestureDetector.SimpleOnGestureListener#onLongPress(android.view.MotionEvent)
		 */
		@Override
		public void onLongPress(MotionEvent e) {

			if (isTouchEvent(e) == false)
				return;

			if(Utils.DEBUG()) Log.d(TAG, "Input: long press");

			showZoomer(true);

			vncCanvas.performHapticFeedback(HapticFeedbackConstants.LONG_PRESS,
					HapticFeedbackConstants.FLAG_IGNORE_GLOBAL_SETTING|HapticFeedbackConstants.FLAG_IGNORE_VIEW_SETTING);
			dragMode = true;
			dragX = e.getX();
			dragY = e.getY();

			// only interpret as button down if virtual mouse buttons are disabled
			if(mousebuttons.getVisibility() != View.VISIBLE)
				dragModeButtonDown = true;
		}

		/*
		 * (non-Javadoc)
		 *
		 * @see android.view.GestureDetector.SimpleOnGestureListener#onScroll(android.view.MotionEvent,
		 *      android.view.MotionEvent, float, float)
		 */
		@Override
		public boolean onScroll(MotionEvent e1, MotionEvent e2,
				float distanceX, float distanceY) {

			if (isTouchEvent(e1) == false)
				return false;

			if (isTouchEvent(e2) == false)
				return false;

			if (e2.getPointerCount() > 1)
			{
				if(Utils.DEBUG()) Log.d(TAG, "Input: scroll multitouch");

				if (inScaling)
					return false;

				// pan on 3 fingers and more
				if(e2.getPointerCount() > 2) {
					showZoomer(false);
					return vncCanvas.pan((int) distanceX, (int) distanceY);
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
				if(twoFingerFlingDetected == false) // not yet detected in this sequence (we only want ONE event per sequence!)
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
				float deltaX = -distanceX *vncCanvas.getScale();
				float deltaY = -distanceY *vncCanvas.getScale();
				deltaX = fineCtrlScale(deltaX);
				deltaY = fineCtrlScale(deltaY);

				// compute the absolution new mouse pos on the remote site.
				float newRemoteX = vncCanvas.mouseX + deltaX;
				float newRemoteY = vncCanvas.mouseY + deltaY;

				if(Utils.DEBUG()) Log.d(TAG, "Input: scroll single touch from "
						+ vncCanvas.mouseX + "," + vncCanvas.mouseY
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
				vncCanvas.processPointerEvent(e2, false);
			}
			return false;
		}


		public boolean onTouchEvent(MotionEvent e) {
			if (isTouchEvent(e) == false) { // physical input device

				e = vncCanvas.changeTouchCoordinatesToFullFrame(e);

				// modify MotionEvent to support Samsung S Pen Event and activate rightButton accordingly
				boolean rightButton = spenActionConvert(e);

				vncCanvas.processPointerEvent(e, true, rightButton);
				vncCanvas.panToMouse();

				return true;
			}

			if (dragMode) {

				if(Utils.DEBUG()) Log.d(TAG, "Input: touch dragMode");

				// compute the relative movement offset on the remote screen.
				float deltaX = (e.getX() - dragX) *vncCanvas.getScale();
				float deltaY = (e.getY() - dragY) *vncCanvas.getScale();
				dragX = e.getX();
				dragY = e.getY();
				deltaX = fineCtrlScale(deltaX);
				deltaY = fineCtrlScale(deltaY);

				// compute the absolution new mouse pos on the remote site.
				float newRemoteX = vncCanvas.mouseX + deltaX;
				float newRemoteY = vncCanvas.mouseY + deltaY;


				if (e.getAction() == MotionEvent.ACTION_UP)
				{
					if(Utils.DEBUG()) Log.d(TAG, "Input: touch dragMode, finger up");

					dragMode = false;

					dragModeButtonDown = false;
					dragModeButton2insteadof1 = false;

					remoteMouseStayPut(e);
					vncCanvas.processPointerEvent(e, false);
					scaleGestures.onTouchEvent(e);
					return gestures.onTouchEvent(e); // important! otherwise the gesture detector gets confused!
				}
				e.setLocation(newRemoteX, newRemoteY);

				boolean status = false;
				if(dragModeButtonDown) {
					if(!dragModeButton2insteadof1) // button 1 down
						status = vncCanvas.processPointerEvent(e, true, false);
					else // button2 down
						status = vncCanvas.processPointerEvent(e, true, true);
				}
				else { // dragging without any button down
					status = vncCanvas.processPointerEvent(e, false);
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
		 * @param e
		 * @return
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
			int action = MotionEvent.ACTION_MASK;
			boolean button = false;
			boolean secondary = false;

			if (isTouchEvent(e)) {
				return false;
			}

			e = vncCanvas.changeTouchCoordinatesToFullFrame(e);

				//Translate the event into onTouchEvent type language
				if (e.getButtonState() != 0) {
					if ((e.getButtonState() & MotionEvent.BUTTON_PRIMARY) != 0) {
						button = true;
						secondary = false;
						action = MotionEvent.ACTION_DOWN;
					} else if ((e.getButtonState() & MotionEvent.BUTTON_SECONDARY) != 0) {
						button = true;
						secondary = true;
						action = MotionEvent.ACTION_DOWN;
					}
					if (e.getAction() == MotionEvent.ACTION_MOVE) {
						action = MotionEvent.ACTION_MOVE;
					}
				} else if ((e.getActionMasked() == MotionEvent.ACTION_HOVER_MOVE) ||
						(e.getActionMasked() == MotionEvent.ACTION_UP)){
					action = MotionEvent.ACTION_UP;
					button = false;
					secondary = false;
				}

				if (action != MotionEvent.ACTION_MASK) {
					e.setAction(action);
					if (button == false) {
						vncCanvas.processPointerEvent(e, false);
					}
					else {
						vncCanvas.processPointerEvent(e, true, secondary);
					}
					vncCanvas.panToMouse();
				}

				if(e.getAction() == MotionEvent.ACTION_SCROLL) {
					if (e.getAxisValue(MotionEvent.AXIS_VSCROLL) < 0.0f) {
						if (Utils.DEBUG()) Log.d(TAG, "Input: scroll down");
						vncCanvas.vncConn.sendPointerEvent(vncCanvas.mouseX, vncCanvas.mouseY, e.getMetaState(), VNCConn.MOUSE_BUTTON_SCROLL_DOWN);
					}
					else {
						if (Utils.DEBUG()) Log.d(TAG, "Input: scroll up");
						vncCanvas.vncConn.sendPointerEvent(vncCanvas.mouseX, vncCanvas.mouseY, e.getMetaState(), VNCConn.MOUSE_BUTTON_SCROLL_UP);
					}
				}

			if(Utils.DEBUG())
				Log.d(TAG, "Input: touch normal: x:" + e.getX() + " y:" + e.getY() + " action:" + e.getAction());

			return true;
		}



		/**
		 * Modify the event so that it does not move the mouse on the
		 * remote server.
		 * @param e
		 */
		private void remoteMouseStayPut(MotionEvent e) {
			e.setLocation(vncCanvas.mouseX, vncCanvas.mouseY);

		}
		/*
		 * (non-Javadoc)
		 * confirmed single tap: do a left mouse down and up click on remote without moving the mouse.
		 * @see android.view.GestureDetector.SimpleOnGestureListener#onSingleTapConfirmed(android.view.MotionEvent)
		 */
		@Override
		public boolean onSingleTapConfirmed(MotionEvent e) {

			if (isTouchEvent(e) == false)
				return false;

			// disable if virtual mouse buttons are in use
			if(mousebuttons.getVisibility()== View.VISIBLE)
				return false;

			if(Utils.DEBUG()) Log.d(TAG, "Input: single tap");

			boolean multiTouch = e.getPointerCount() > 1;
			remoteMouseStayPut(e);

			vncCanvas.processPointerEvent(e, true, multiTouch||vncCanvas.cameraButtonDown);
			e.setAction(MotionEvent.ACTION_UP);
			return vncCanvas.processPointerEvent(e, false, multiTouch||vncCanvas.cameraButtonDown);
		}

		/*
		 * (non-Javadoc)
		 * double tap: do right mouse down and up click on remote without moving the mouse.
		 * @see android.view.GestureDetector.SimpleOnGestureListener#onDoubleTap(android.view.MotionEvent)
		 */
		@Override
		public boolean onDoubleTap(MotionEvent e) {

			if (isTouchEvent(e) == false)
				return false;

			// disable if virtual mouse buttons are in use
			if(mousebuttons.getVisibility()== View.VISIBLE)
				return false;

			if(Utils.DEBUG()) Log.d(TAG, "Input: double tap");

			dragModeButtonDown = true;
			dragModeButton2insteadof1 = true;

			remoteMouseStayPut(e);
			vncCanvas.processPointerEvent(e, true, true);
			e.setAction(MotionEvent.ACTION_UP);
			return vncCanvas.processPointerEvent(e, false, true);
		}


		/*
		 * (non-Javadoc)
		 *
		 * @see android.view.GestureDetector.SimpleOnGestureListener#onDown(android.view.MotionEvent)
		 */
		@Override
		public boolean onDown(MotionEvent e) {
			if (isTouchEvent(e) == false)
				return false;

			return true;
		}
	}

	private final static String TAG = "VncCanvasActivity";


	VncCanvas vncCanvas;
	VncDatabase database;
	private ConnectionBean connection;

	ZoomControls zoomer;
	TextView zoomLevel;
	MightyInputHandler inputHandler;

	ViewGroup mousebuttons;
	TouchPointView touchpoints;
	Toast notificationToast;
	PopupMenu fabMenu;

	ProgressDialog firstFrameWaitDialog;

	private SharedPreferences prefs;

	private ClipboardManager mClipboardManager;

	@SuppressLint("ShowToast")
	@Override
	public void onCreate(Bundle savedInstanceState) {

		super.onCreate(savedInstanceState);

		// hide title bar, status bar
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
				WindowManager.LayoutParams.FLAG_FULLSCREEN);

		// hide system ui after softkeyboard close as per https://stackoverflow.com/a/21278040/361413
		final View decorView = getWindow().getDecorView();
		decorView.setOnSystemUiVisibilityChangeListener (new View.OnSystemUiVisibilityChangeListener() {
			@Override
			public void onSystemUiVisibilityChange(int visibility) {
				hideSystemUI();
			}
		});

		// Setup resizing when keyboard is visible.
		//
		// Ideally, we would let Android manage resizing but because we are using a fullscreen window,
		// most of the "normal" options don't work for us.
		//
		// We have to hook into layout phase and manually shift our view up by adding appropriate
		// bottom padding.
		final View contentView = findViewById(android.R.id.content);
		contentView.getViewTreeObserver().addOnGlobalLayoutListener(() -> {
			Rect frame = new Rect();
			contentView.getWindowVisibleDisplayFrame(frame);

			int contentBottom = contentView.getBottom();
			int paddingBottom = contentBottom - frame.bottom;

			if (paddingBottom < 0)
				paddingBottom = 0;

			//When padding is less then 20% of height, it is most probably navigation bar.
			if (paddingBottom > 0 && paddingBottom < contentBottom * .20)
				return;

			contentView.setPadding(0, 0, 0, paddingBottom); //Update bottom
		});

		setContentView(R.layout.canvas);

		vncCanvas = (VncCanvas) findViewById(R.id.vnc_canvas);
		zoomer = (ZoomControls) findViewById(R.id.zoomer);
		zoomLevel = findViewById(R.id.zoomLevel);

		prefs = getSharedPreferences(Constants.PREFSNAME, MODE_PRIVATE);

		database = VncDatabase.getInstance(this);

		mClipboardManager = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);


		inputHandler = new MightyInputHandler();
		inputHandler.init();

		/*
		 * Setup floating action button & associated menu
		 */
		FloatingActionButton fab = findViewById(R.id.fab);
		fab.setOnClickListener(view -> {
			Log.d(TAG, "FAB onClick");
			prepareFabMenu(fabMenu);
			fabMenu.show();
		});
		fabMenu = new PopupMenu(this, fab);
		fabMenu.inflate(R.menu.vnccanvasactivitymenu);
		fabMenu.setOnMenuItemClickListener(this);


		/*
		 * Setup connection bean.
		 */
		connection = new ConnectionBean();
		Intent i = getIntent();
		Uri data = i.getData();
		if ((data != null) && (data.getScheme().equals("vnc"))) { // started from outer world

			Log.d(TAG, "Starting via vnc://");

			// This should not happen according to Uri contract, but bug introduced in Froyo (2.2)
			// has made this parsing of host necessary, i.e. getPort() returns -1 and the stuff after the colon is
			// still in the host part...
			// http://code.google.com/p/android/issues/detail?id=9952
			if(! connection.parseHostPort(data.getHost())) {
				// no colons in getHost()
				connection.port = data.getPort();
				connection.address = data.getHost();
			}

			if (connection.address.equals(Constants.CONNECTION)) // this is a bookmarked connection
			{
				Log.d(TAG, "Starting bookmarked connection " + connection.port);
				// read in this bookmarked connection
				ConnectionBean result = database.getConnectionDao().get(connection.port);
				if (result == null) {
					Log.e(TAG, "Bookmarked connection " + connection.port + " does not exist!");
					Utils.showFatalErrorMessage(this, getString(R.string.bookmark_invalid));
					return;
				}
				connection = result;
			}
			else // well, not a boomarked connection
			{
				connection.nickname = connection.address;
				List<String> path = data.getPathSegments();
			    if (path.size() >= 1) {
					connection.colorModel = path.get(0);
				}
			    if (path.size() >= 2) {
					connection.password = path.get(1);
				}
			}
		}
		// Uri == null
		else { // i.e. started from main menu

		    Bundle extras = i.getExtras();

		    if (extras != null) {
		  	    connection.Gen_populate((ContentValues) extras
				  	.getParcelable(Constants.CONNECTION));
		    }
			if (connection.port == 0)
				connection.port = 5900;

			Log.d(TAG, "Got raw intent " + connection.toString());

			// Parse a HOST:PORT entry
			connection.parseHostPort(connection.address);
		}


		/*
		 * Setup canvas and conn.
		 */
		VNCConn conn = new VNCConn();
		vncCanvas.initializeVncCanvas(this, inputHandler, conn); // add conn to canvas
		conn.setCanvas(vncCanvas); // add canvas to conn. be sure to call this before init!
		// the actual connection init
		conn.init(connection, new Runnable() {
			public void run() {
				setModes();
			}
		});





		zoomer.hide();
		zoomer.setOnZoomInClickListener(new View.OnClickListener() {

			/*
			 * (non-Javadoc)
			 *
			 * @see android.view.View.OnClickListener#onClick(android.view.View)
			 */
			@Override
			public void onClick(View v) {
				try {
					showZoomer(true);
					vncCanvas.scaling.zoomIn();
				}
				catch(NullPointerException e) {
				}
			}

		});
		zoomer.setOnZoomOutClickListener(new View.OnClickListener() {

			/*
			 * (non-Javadoc)
			 *
			 * @see android.view.View.OnClickListener#onClick(android.view.View)
			 */
			@Override
			public void onClick(View v) {
				try {
					showZoomer(true);
					vncCanvas.scaling.zoomOut();
				}
				catch(NullPointerException e) {
				}
			}

		});
		zoomer.setOnZoomKeyboardClickListener(new View.OnClickListener() {

			/*
			 * (non-Javadoc)
			 *
			 * @see android.view.View.OnClickListener#onClick(android.view.View)
			 */
			@Override
			public void onClick(View v) {
				toggleKeyboard();
			}

		});

		mousebuttons = (ViewGroup) findViewById(R.id.virtualmousebuttons);
		MouseButtonView mousebutton1 = (MouseButtonView) findViewById(R.id.mousebutton1);
		MouseButtonView mousebutton2 = (MouseButtonView) findViewById(R.id.mousebutton2);
		MouseButtonView mousebutton3 = (MouseButtonView) findViewById(R.id.mousebutton3);

		mousebutton1.init(1, vncCanvas);
		mousebutton2.init(2, vncCanvas);
		mousebutton3.init(3, vncCanvas);
		if(! prefs.getBoolean(Constants.PREFS_KEY_MOUSEBUTTONS, true))
			mousebuttons.setVisibility(View.GONE);

		touchpoints = (TouchPointView) findViewById(R.id.touchpoints);
		touchpoints.setInputHandler(inputHandler);

		// create an empty toast. we do this do be able to cancel
		notificationToast = Toast.makeText(this,  "", Toast.LENGTH_SHORT);
		notificationToast.setGravity(Gravity.TOP, 0, 60);


		if(! prefs.getBoolean(Constants.PREFS_KEY_POINTERHIGHLIGHT, true))
			vncCanvas.setPointerHighlight(false);


		/*
		 * ask whether to show help on first run
		 */
		if(Utils.appstarts == 1) {
			new AlertDialog.Builder(this)
			.setMessage(R.string.firstrun_help_dialog_text)
			.setTitle(R.string.firstrun_help_dialog_title)
			.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int id) {
					Intent helpIntent = new Intent (VncCanvasActivity.this, HelpActivity.class);
					VncCanvasActivity.this.startActivity(helpIntent);
				}
			})
			.setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int id) {
					try {
						dialog.dismiss();
					}
					catch(Exception e) {
					}
				}
			})
			.show();
		}

	}

	/**
	 * Set modes on start to match what is specified in the ConnectionBean;
	 * color mode (already done), scaling
	 */
	void setModes() {
		float minScale = vncCanvas.getMinimumScale();
		vncCanvas.scaling = new ZoomScaling(this, minScale, 4);
	}

	ConnectionBean getConnection() {
		return connection;
	}

	/*
	 * (non-Javadoc)
	 *
	 * @see android.app.Activity#onCreateDialog(int)
	 */
	@Override
	protected Dialog onCreateDialog(int id) {


		// Default to meta key dialog
		return new MetaKeyDialog(this);
	}

	/*
	 * (non-Javadoc)
	 *
	 * @see android.app.Activity#onPrepareDialog(int, android.app.Dialog)
	 */
	@Override
	protected void onPrepareDialog(int id, Dialog dialog) {
		super.onPrepareDialog(id, dialog);
		if (dialog instanceof ConnectionSettable)
			((ConnectionSettable) dialog).setConnection(connection);
	}


	@Override
	protected void onStop() {
		vncCanvas.disableRepaints();
		super.onStop();
	}

	@Override
	protected void onRestart() {
		vncCanvas.enableRepaints();
		super.onRestart();
	}


	@Override
	protected void onPause() {
		super.onPause();
		// needed for the GLSurfaceView
		vncCanvas.onPause();

		// get VNC cuttext and post to Android
		if(vncCanvas.vncConn.getCutText() != null) {
			try {
				mClipboardManager.setText(vncCanvas.vncConn.getCutText());
			} catch (Exception e) {
				//unused
			}
		}

	}

	@Override
	protected void onResume() {
		super.onResume();
		// needed for the GLSurfaceView
		vncCanvas.onResume();

		// get Android clipboard contents
		if (mClipboardManager.hasText()) {
			try {
				vncCanvas.vncConn.sendCutText(mClipboardManager.getText().toString());
			}
			catch(NullPointerException e) {
				//unused
			}
		}

	}

	/**
	 * Prepare FAB popup menu.
	 */
	private void prepareFabMenu(PopupMenu popupMenu) {
		Menu menu = popupMenu.getMenu();
		if (touchpoints.getVisibility() == View.VISIBLE) {
			menu.findItem(R.id.itemColorMode).setVisible(false);
			menu.findItem(R.id.itemTogglePointerHighlight).setVisible(false);
		} else {
			menu.findItem(R.id.itemColorMode).setVisible(true);
			menu.findItem(R.id.itemTogglePointerHighlight).setVisible(true);
		}

		// changing pixel format without Fence extension (https://github.com/rfbproto/rfbproto/blob/master/rfbproto.rst#clientfence) not safely possible
		menu.findItem(R.id.itemColorMode).setVisible(false);
	}

	@Override
	public boolean onMenuItemClick(MenuItem item) {

		SharedPreferences.Editor ed = prefs.edit();

		switch (item.getItemId()) {
		case R.id.itemInfo:
			vncCanvas.showConnectionInfo();
			return true;
		case R.id.itemSpecialKeys:
			showDialog(R.layout.metakey);
			return true;
		case R.id.itemColorMode:
			selectColorModel();
			return true;
		case R.id.itemToggleFramebufferUpdate:
			if(vncCanvas.vncConn.toggleFramebufferUpdates()) // view enabled
			{
				vncCanvas.setVisibility(View.VISIBLE);
				touchpoints.setVisibility(View.GONE);
			}
			else
			{
				vncCanvas.setVisibility(View.GONE);
				touchpoints.setVisibility(View.VISIBLE);
			}
			return true;

		case R.id.itemToggleMouseButtons:
			if(mousebuttons.getVisibility()== View.VISIBLE) {
				mousebuttons.setVisibility(View.GONE);
				ed.putBoolean(Constants.PREFS_KEY_MOUSEBUTTONS, false);
			}
			else {
				mousebuttons.setVisibility(View.VISIBLE);
				ed.putBoolean(Constants.PREFS_KEY_MOUSEBUTTONS, true);
			}
			ed.commit();
			return true;

		case R.id.itemTogglePointerHighlight:
			if(vncCanvas.getPointerHighlight())
				vncCanvas.setPointerHighlight(false);
			else
				vncCanvas.setPointerHighlight(true);

			ed.putBoolean(Constants.PREFS_KEY_POINTERHIGHLIGHT, vncCanvas.getPointerHighlight());
			ed.commit();
			return true;

		case R.id.itemToggleKeyboard:
			toggleKeyboard();
			return true;

		case R.id.itemSendKeyAgain:
			sendSpecialKeyAgain();
			return true;
		case R.id.itemSaveBookmark:
			database.getConnectionDao().save(connection);
			Toast.makeText(this, getString(R.string.bookmark_saved), Toast.LENGTH_SHORT).show();
			return true;
		case R.id.itemAbout:
			Intent intent = new Intent (this, AboutActivity.class);
			this.startActivity(intent);
			return true;
		case R.id.itemHelp:
			Intent helpIntent = new Intent (this, HelpActivity.class);
			this.startActivity(helpIntent);
			return true;
		case R.id.itemDisconnect:
			new AlertDialog.Builder(this)
			.setMessage(getString(R.string.disconnect_question))
			.setPositiveButton(getString(android.R.string.yes), new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int whichButton) {
					vncCanvas.vncConn.shutdown();
					finish();
				}
			}).setNegativeButton(getString(android.R.string.no), new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int whichButton) {
					// Do nothing.
				}
			}).show();
			return true;
		default:
			break;
		}
		return super.onOptionsItemSelected(item);
	}

	MetaKeyBean lastSentKey;

	private void sendSpecialKeyAgain() {
		if (lastSentKey == null) {
			lastSentKey = database.getMetaKeyDao().get(connection.lastMetaKeyId);
		}
		vncCanvas.sendMetaKey(lastSentKey);
	}

	private void toggleKeyboard() {
		InputMethodManager inputMgr = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
		vncCanvas.requestFocus();
		inputMgr.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		if (isFinishing()) {
			try {
				inputHandler.shutdown();
				vncCanvas.vncConn.shutdown();
				vncCanvas.onDestroy();
			}
			catch(NullPointerException e) {
			}
		}
	}


	@Override
	public boolean onKeyDown(int keyCode, KeyEvent evt) {
		if(Utils.DEBUG()) Log.d(TAG, "Input: key down: " + evt.toString());

		if (keyCode == KeyEvent.KEYCODE_MENU) {
			prepareFabMenu(fabMenu);
			fabMenu.show();
			return true;
		}

		if(keyCode == KeyEvent.KEYCODE_BACK) {

			// handle right mouse button of USB-OTG devices
			// Also, https://fossies.org/linux/SDL2/android-project/app/src/main/java/org/libsdl/app/SDLActivity.java line 1943 states:
			// 12290 = Samsung DeX mode desktop mouse
			// 12290 = 0x3002 = 0x2002 | 0x1002 = SOURCE_MOUSE | SOURCE_TOUCHSCREEN
			if(evt.getSource() == InputDevice.SOURCE_MOUSE || evt.getSource() == (InputDevice.SOURCE_MOUSE | InputDevice.SOURCE_TOUCHSCREEN)) {
				MotionEvent e = MotionEvent.obtain(
						SystemClock.uptimeMillis(),
						SystemClock.uptimeMillis(),
						MotionEvent.ACTION_DOWN,
						vncCanvas.mouseX,
						vncCanvas.mouseY,
						0
						);
				vncCanvas.processPointerEvent(e, true, true);
				return true;
			}

			if(evt.getFlags() == KeyEvent.FLAG_FROM_SYSTEM) // from hardware keyboard
				keyCode = KeyEvent.KEYCODE_ESCAPE;
			else {
				new AlertDialog.Builder(this)
				.setMessage(getString(R.string.disconnect_question))
				.setPositiveButton(getString(android.R.string.yes), new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int whichButton) {
						vncCanvas.vncConn.shutdown();
						finish();
					}
				}).setNegativeButton(getString(android.R.string.no), new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int whichButton) {
						// Do nothing.
					}
				}).show();
				return true;
			}
		}

		// use search key to toggle soft keyboard
		if (keyCode == KeyEvent.KEYCODE_SEARCH)
			toggleKeyboard();

		if (vncCanvas.processLocalKeyEvent(keyCode, evt))
			return true;
		return super.onKeyDown(keyCode, evt);
	}

	@Override
	public boolean onKeyUp(int keyCode, KeyEvent evt) {
		if(Utils.DEBUG()) Log.d(TAG, "Input: key up: " + evt.toString());

		if(keyCode == KeyEvent.KEYCODE_BACK) {
			// handle right mouse button of USB-OTG devices
			if (evt.getSource() == InputDevice.SOURCE_MOUSE) {
				MotionEvent e = MotionEvent.obtain(
						SystemClock.uptimeMillis(),
						SystemClock.uptimeMillis(),
						MotionEvent.ACTION_UP,
						vncCanvas.mouseX,
						vncCanvas.mouseY,
						0
				);
				vncCanvas.processPointerEvent(e, false, true);
				return true;
			}
		}

		if (keyCode == KeyEvent.KEYCODE_MENU)
			return super.onKeyUp(keyCode, evt);

		if (vncCanvas.processLocalKeyEvent(keyCode, evt))
			return true;
		return super.onKeyUp(keyCode, evt);
	}


	// this is called for unicode symbols like €
	// multiple duplicate key events have occurred in a row, or a complex string is being delivered.
	// If the key code is not KEYCODE_UNKNOWN then the getRepeatCount() method returns the number of
	// times the given key code should be executed.
	// Otherwise, if the key code is KEYCODE_UNKNOWN, then this is a sequence of characters as returned by getCharacters().
	@Override
	public boolean onKeyMultiple (int keyCode, int count, KeyEvent evt) {
		if(Utils.DEBUG()) Log.d(TAG, "Input: key mult: " + evt.toString());

		// we only deal with the special char case for now
		if(evt.getKeyCode() == KeyEvent.KEYCODE_UNKNOWN) {
			if (vncCanvas.processLocalKeyEvent(keyCode, evt))
				return true;
		}

		return super.onKeyMultiple(keyCode, count, evt);
	}





	private void selectColorModel() {
		// Stop repainting the desktop
		// because the display is composited!
		vncCanvas.disableRepaints();

		final String[] choices = new String[COLORMODEL.values().length];
		int currentSelection = -1;
		for (int i = 0; i < choices.length; i++) {
			COLORMODEL cm = COLORMODEL.values()[i];
			choices[i] = cm.toString();
			if(cm.equals(vncCanvas.vncConn.getColorModel()))
				currentSelection = i;
		}

		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		builder.setSingleChoiceItems(choices, currentSelection, new DialogInterface.OnClickListener() {
		    public void onClick(DialogInterface dialog, int item) {
		    	try{
					dialog.dismiss();
				}
				catch(Exception e) {
				}
				COLORMODEL cm = COLORMODEL.values()[item];
				vncCanvas.vncConn.setColorModel(cm);
				connection.colorModel = cm.nameString();
				Toast.makeText(VncCanvasActivity.this,
						"Updating Color Model to " + cm.toString(),
						Toast.LENGTH_SHORT).show();
		    }
		});
		AlertDialog dialog = builder.create();
		dialog.setOnDismissListener(new OnDismissListener() {
			@Override
			public void onDismiss(DialogInterface arg0) {
				Log.i(TAG, "Color Model Selector dismissed");
				// Restore desktop repaints
				vncCanvas.enableRepaints();
			}
		});
		dialog.show();
	}

	static final long ZOOM_HIDE_DELAY_MS = 2500;
	Runnable hideZoomInstance = () -> zoomer.hide();

	private void showZoomer(boolean force) {
		if (force || zoomer.getVisibility() != View.VISIBLE) {
			zoomer.show();

			//Schedule hiding of zoom controls.
			vncCanvas.handler.removeCallbacks(hideZoomInstance);
			vncCanvas.handler.postDelayed(hideZoomInstance, ZOOM_HIDE_DELAY_MS);
		}
	}

	Runnable hideZoomLevelInstance = () -> zoomLevel.setVisibility(View.INVISIBLE);
	public void showZoomLevel()
	{
		zoomLevel.setText("" + (int)(vncCanvas.getScale()*100) +"%");
		zoomLevel.setVisibility(View.VISIBLE);
		vncCanvas.handler.removeCallbacks(hideZoomLevelInstance);
		vncCanvas.handler.postDelayed(hideZoomLevelInstance, ZOOM_HIDE_DELAY_MS);

		//Workaround for buggy GLSurfaceView.
		//See https://stackoverflow.com/questions/11236336/setvisibilityview-visible-doesnt-always-work-ideas
		zoomLevel.requestLayout();
	}


	@Override
	public void onWindowFocusChanged(boolean hasFocus) {
		super.onWindowFocusChanged(hasFocus);
		if (hasFocus) {
			hideSystemUI();
		}
	}

	private void hideSystemUI() {
		// For "lean back" mode, remove SYSTEM_UI_FLAG_IMMERSIVE.
		// Or for "sticky immersive," replace it with SYSTEM_UI_FLAG_IMMERSIVE_STICKY
		View decorView = getWindow().getDecorView();
		decorView.setSystemUiVisibility(
						View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
						// Set the content to appear under the system bars so that the
						// content doesn't resize when the system bars hide and show.
						| View.SYSTEM_UI_FLAG_LAYOUT_STABLE
						| View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
						| View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
						// Hide the nav bar and status bar
						| View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
						| View.SYSTEM_UI_FLAG_FULLSCREEN);
	}

}
