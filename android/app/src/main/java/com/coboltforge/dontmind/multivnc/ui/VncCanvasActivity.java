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
package com.coboltforge.dontmind.multivnc.ui;

import java.util.List;


import android.Manifest;
import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.os.Build;
import android.text.ClipboardManager;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.DialogInterface.OnDismissListener;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Bundle;
import android.os.SystemClock;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.EditText;
import android.widget.PopupMenu;
import android.widget.TextView;
import android.widget.Toast;
import android.view.inputmethod.InputMethodManager;
import android.content.Context;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AppCompatActivity;

import com.coboltforge.dontmind.multivnc.COLORMODEL;
import com.coboltforge.dontmind.multivnc.Constants;
import com.coboltforge.dontmind.multivnc.R;
import com.coboltforge.dontmind.multivnc.Utils;
import com.coboltforge.dontmind.multivnc.VNCConn;
import com.coboltforge.dontmind.multivnc.db.ConnectionBean;
import com.coboltforge.dontmind.multivnc.db.MetaKeyBean;
import com.coboltforge.dontmind.multivnc.db.VncDatabase;
import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.google.android.material.textfield.TextInputLayout;

@SuppressWarnings("deprecation")
public class VncCanvasActivity extends AppCompatActivity implements PopupMenu.OnMenuItemClickListener {


	private final static String TAG = "VncCanvasActivity";


	VncCanvas vncCanvas;
	VncDatabase database;
	private ConnectionBean connection;

	ZoomControls zoomer;
	TextView zoomLevel;
	PointerInputHandler inputHandler;

	ViewGroup mousebuttons;
	TouchPointView touchpoints;
	Toast notificationToast;
	PopupMenu fabMenu;

	public ProgressDialog firstFrameWaitDialog;

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

		setContentView(R.layout.canvas_activity);

		vncCanvas = (VncCanvas) findViewById(R.id.vnc_canvas);
		zoomer = (ZoomControls) findViewById(R.id.zoomer);
		zoomLevel = findViewById(R.id.zoomLevel);
		mousebuttons = (ViewGroup) findViewById(R.id.virtualmousebuttons);

		prefs = getSharedPreferences(Constants.PREFSNAME, MODE_PRIVATE);

		database = VncDatabase.getInstance(this);

		mClipboardManager = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);

		// create an empty toast. we do this do be able to cancel
		notificationToast = Toast.makeText(this,  "", Toast.LENGTH_SHORT);
		notificationToast.setGravity(Gravity.TOP, 0, 60);

		inputHandler = new PointerInputHandler(vncCanvas, mousebuttons, notificationToast);
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
		  	    connection = extras.getParcelable(Constants.CONNECTION);
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
		VNCConn conn = new VNCConn(vncCanvas, vncCanvas);
		// the Android 13 permission launcher and callback handler
		ActivityResultLauncher<String> requestPermissionLauncher =
				registerForActivityResult(new ActivityResultContracts.RequestPermission(), isGranted -> {
					// show UI _after_ all the permission handling
					showHelpDialog();
					VNCConnService.register(VncCanvasActivity.this, conn);
					vncCanvas.showConnectionInfo();
				});
		// the actual connection init
		// Startup the VNCConn with a nifty progress dialog
		final ProgressDialog pd = new ProgressDialog(this);
		pd.setCancelable(false); // on ICS, clicking somewhere cancels the dialog. not what we want...
		pd.setTitle("Connecting...");
		pd.setMessage("Establishing handshake.\nPlease wait...");
		pd.setButton(DialogInterface.BUTTON_NEGATIVE, getString(android.R.string.cancel), (dialog, which) -> finish());
		pd.show();
		firstFrameWaitDialog = pd;
		vncCanvas.initializeVncCanvas(pd, inputHandler, conn); // add conn to canvas
		conn.init(connection,
				// onInit
				initError -> runOnUiThread(() -> {
					if(isFinishing()) {
						// this can happen when the activity is finishing, but the runOnUiThread comes later
						Log.w(TAG, "VNCConn's onInit run while finishing activity, doing nothing");
						return;
					}

					if(initError == null) {
						// init success!
						if (Build.VERSION.SDK_INT < 33) {
							/*
							 * Show all the on-connect UI directly
							 */
							showHelpDialog();
							VNCConnService.register(VncCanvasActivity.this, conn);
							vncCanvas.showConnectionInfo();
						} else {
							/*
								permission asking according to the book https://developer.android.com/training/permissions/requesting
							*/
							// the permission asking logic as per the book https://developer.android.com/training/permissions/requesting
							if (checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) == PackageManager.PERMISSION_GRANTED) {
								showHelpDialog();
								VNCConnService.register(VncCanvasActivity.this, conn);
								vncCanvas.showConnectionInfo();
							} else if (shouldShowRequestPermissionRationale(Manifest.permission.POST_NOTIFICATIONS)) {
								new AlertDialog.Builder(VncCanvasActivity.this)
										.setCancelable(false)
										.setTitle(R.string.notification_title)
										.setMessage(R.string.notification_msg)
										.setPositiveButton(android.R.string.ok, (dialog, which) -> {
											requestPermissionLauncher.launch(Manifest.permission.POST_NOTIFICATIONS);
										})
										.setCancelable(false)
										.show();
							} else {
								// You can directly ask for the permission.
								// The registered ActivityResultCallback gets the result of this request.
								requestPermissionLauncher.launch(Manifest.permission.POST_NOTIFICATIONS);
							}
						}

						setTitle(conn.getDesktopName());
						// actually set scale type with this, otherwise no scaling
						setModes();
						firstFrameWaitDialog.setMessage("Downloading first frame.\nPlease wait...");
						// center pointer
						vncCanvas.mouseX = conn.getFramebufferWidth() / 2;
						vncCanvas.mouseY = conn.getFramebufferHeight() / 2;
					} else {
						// init fail
						try {
							firstFrameWaitDialog.dismiss();
						} catch (Exception ignored) {
						}
						String error = "VNC connection setup failed!";
						if (initError.getMessage() != null && (initError.getMessage().contains("authentication"))) {
							error = "VNC authentication failed!";
						}
						final String error_ = error + "<br>" + ((initError.getLocalizedMessage() != null) ? initError.getLocalizedMessage() : "");
						Utils.showFatalErrorMessage(VncCanvasActivity.this, error_);
					}
				}),
				// onDisconnect
				disconnectError -> runOnUiThread(() -> {
					try {
						// Ensure we dismiss the progress dialog
						// before we fatal error finish
						if (firstFrameWaitDialog.isShowing())
							firstFrameWaitDialog.dismiss();
					} catch (Exception e) {
						//unused
					}

					if(disconnectError != null) {
						String error = "VNC connection failed!";
						final String error_ = error + "<br>" + ((disconnectError.getLocalizedMessage() != null) ? disconnectError.getLocalizedMessage() : "");
						Utils.showFatalErrorMessage(VncCanvasActivity.this, error_);
					}

					// deregister connection
					VNCConnService.deregister(VncCanvasActivity.this, conn);
				}));

		zoomer.setOnZoomInClickListener(new View.OnClickListener() {

			/*
			 * (non-Javadoc)
			 *
			 * @see android.view.View.OnClickListener#onClick(android.view.View)
			 */
			@Override
			public void onClick(View v) {
				try {
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


		if(! prefs.getBoolean(Constants.PREFS_KEY_POINTERHIGHLIGHT, true))
			vncCanvas.setPointerHighlight(false);
	}

	/**
	 * Set modes on start to match what is specified in the ConnectionBean;
	 * color mode (already done), scaling
	 */
	public void setModes() {
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
		if (dialog instanceof MetaKeyDialog)
			((MetaKeyDialog) dialog).setConnection(connection);
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
			showDialog(R.layout.metakey_dialog);
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

			if(connection.id != 0) {
				// a bookmarked connection. save it right away
				database.getConnectionDao().save(connection);
				Toast.makeText(VncCanvasActivity.this, getString(R.string.bookmark_saved), Toast.LENGTH_SHORT).show();
				return true;
			}

			final String defaultName = connection.address + ":" + connection.port;

			final EditText input = new EditText(this);
			input.setHint(defaultName);
			TextInputLayout inputLayout = new TextInputLayout(this);
			inputLayout.setPadding(
					(int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 20, Resources.getSystem().getDisplayMetrics()),
					0,
					(int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 20, Resources.getSystem().getDisplayMetrics()),
					0
			);
			inputLayout.addView(input);

			new AlertDialog.Builder(this)
					.setMessage(getString(R.string.enterbookmarkname))
					.setView(inputLayout)
					.setPositiveButton(getString(android.R.string.ok), (dialog, whichButton) -> {
						String name = input.getText().toString();
						if(name.length() == 0)
							name = defaultName;
						connection.nickname = name;
						database.getConnectionDao().save(connection);
						Toast.makeText(VncCanvasActivity.this, getString(R.string.bookmark_saved), Toast.LENGTH_SHORT).show();
					}).setNegativeButton(getString(android.R.string.cancel), (dialog, whichButton) -> {
						// Do nothing.
					}).show();
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
		// Android 12 changed something in respect to soft input, we have to add a delay to be able to show :-/
		vncCanvas.postDelayed(() -> inputMgr.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0), 100);
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


	private void showHelpDialog() {
		SharedPreferences settings = getSharedPreferences(Constants.PREFSNAME, 0);
		if(settings.getBoolean(Constants.PREFS_KEY_FIRST_CONNECTION, true)) {
			new AlertDialog.Builder(this)
					.setMessage(R.string.firstrun_help_dialog_text)
					.setTitle(R.string.firstrun_help_dialog_title)
					.setPositiveButton(android.R.string.ok, (dialog, id) -> {
						Intent helpIntent = new Intent (VncCanvasActivity.this, HelpActivity.class);
						VncCanvasActivity.this.startActivity(helpIntent);
					})
					.setNegativeButton(android.R.string.cancel, (dialog, id) -> {
						try {
							dialog.dismiss();
						}
						catch(Exception ignored) {
						}
					})
					.show();
			// and set this
			SharedPreferences.Editor ed = settings.edit();
			ed.putBoolean(Constants.PREFS_KEY_FIRST_CONNECTION, false);
			ed.apply();
		}
	}

}
