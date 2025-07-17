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
// MainMenuActivity is the Activity for setting VNC server IP and port.
//

package com.coboltforge.dontmind.multivnc.ui;

import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;
import android.text.Html;
import android.text.method.LinkMovementMethod;
import android.util.Log;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleObserver;
import androidx.lifecycle.OnLifecycleEvent;
import androidx.lifecycle.ProcessLifecycleOwner;

import com.coboltforge.dontmind.multivnc.Constants;
import com.coboltforge.dontmind.multivnc.MDNSService;
import com.coboltforge.dontmind.multivnc.R;
import com.coboltforge.dontmind.multivnc.Utils;
import com.coboltforge.dontmind.multivnc.db.ConnectionBean;
import com.coboltforge.dontmind.multivnc.db.VncDatabase;
import com.google.android.material.textfield.TextInputLayout;

import java.util.Collections;
import java.util.Hashtable;
import java.util.List;


public class MainMenuActivity extends AppCompatActivity implements MDNSService.OnEventListener, LifecycleObserver {

	private static final String TAG = "MainMenuActivity";

	private LinearLayout serverlist;
	private LinearLayout bookmarkslist;

	private VncDatabase database;

	// service discovery stuff
	private MDNSService boundMDNSService;
	private ServiceConnection connection_MDNSService;
	private android.os.Handler handler = new android.os.Handler();


	public class MDNSServiceConnection implements ServiceConnection {
		public void onServiceConnected(ComponentName className, IBinder service) {
			// This is called when the connection with the service has been
			// established, giving us the service object we can use to
			// interact with the service. Because we have bound to a explicit
			// service that we know is running in our own process, we can
			// cast its IBinder to a concrete class and directly access it.
			boundMDNSService = ((MDNSService.LocalBinder) service).getService();

			// register our callback to be notified about changes
			boundMDNSService.registerCallback(MainMenuActivity.this);

			// and force a dump of discovered connections
			boundMDNSService.dump();

			Log.d(TAG, "connected to MDNSService " + boundMDNSService);
		}

		public void onServiceDisconnected(ComponentName className) {

			Log.d(TAG, "disconnected from MDNSService " + boundMDNSService);

			// This is called when the connection with the service has been
			// unexpectedly disconnected -- that is, its process crashed.
			// Because it is running in our same process, we should never
			// see this happen.
			boundMDNSService = null;
		}
	};



	@Override
	public void onCreate(Bundle savedInstanceState) {

		super.onCreate(savedInstanceState);
		// On Android 15 and later, calling enableEdgeToEdge ensures system bar icon colors update
		// when the device theme changes. Because calling it on pre-Android 15 has the side effect of
		// enabling EdgeToEdge there as well, we only use it on Android 15 and later.
		if (Build.VERSION.SDK_INT >= 35) {
			EdgeToEdge.enable(this);
		}

		setContentView(R.layout.mainmenu_activity);

		// start the MDNS service
		startMDNSService();
		// and (re-)bind to MDNS service
		bindToMDNSService(new Intent(this, MDNSService.class));

		serverlist = (LinearLayout) findViewById(R.id.discovered_servers_list);
		bookmarkslist = (LinearLayout) findViewById(R.id.bookmarks_list);

		Button goButton = (Button) findViewById(R.id.buttonGO);
		goButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View view) {
				ConnectionEditFragment editor = (ConnectionEditFragment) getSupportFragmentManager().findFragmentById(R.id.connectionEditFragment);
				ConnectionBean conn = editor != null ? editor.getConnection() : null;
				if(conn == null)
					return;
				Log.d(TAG, "Starting NEW connection " + conn.toString());
				Intent intent = new Intent(MainMenuActivity.this, VncCanvasActivity.class);
				intent.putExtra(Constants.CONNECTION,conn);
				startActivity(intent);
			}
		});

		Button saveBookmarkButton = (Button) findViewById(R.id.buttonSaveBookmark);
		saveBookmarkButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				ConnectionEditFragment editor = (ConnectionEditFragment) getSupportFragmentManager().findFragmentById(R.id.connectionEditFragment);
				final ConnectionBean conn = editor != null ? editor.getConnection() : null;
				if(conn == null)
					return;

				final String defaultName = conn.address + ":" + conn.port;

				final EditText input = new EditText(MainMenuActivity.this);
				input.setHint(defaultName);
				TextInputLayout inputLayout = new TextInputLayout(MainMenuActivity.this);
				inputLayout.setPadding(
						(int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 20, Resources.getSystem().getDisplayMetrics()),
					0,
						(int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 20, Resources.getSystem().getDisplayMetrics()),
						0
				);
				inputLayout.addView(input);

				new AlertDialog.Builder(MainMenuActivity.this)
				.setMessage(getString(R.string.enterbookmarkname))
				.setView(inputLayout)
				.setPositiveButton(getString(android.R.string.ok), new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int whichButton) {
						String name = input.getText().toString();
						if(name.length() == 0)
							name = defaultName;
						conn.nickname = name;
						saveBookmark(conn);
						updateBookmarkView();
					}
				}).setNegativeButton(getString(android.R.string.cancel), new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int whichButton) {
						// Do nothing.
					}
				}).show();

			}
		});

		database = VncDatabase.getInstance(this);


		final SharedPreferences settings = getSharedPreferences(Constants.PREFSNAME, MODE_PRIVATE);

		/*
		 * show support dialog on third (and maybe later) runs
		 */
		if(settings.getInt(Constants.PREFS_KEY_MAIN_MENU_STARTS, 0) > 2)
		{
			if(settings.getBoolean(Constants.PREFS_KEY_SUPPORTDLG, true) && savedInstanceState == null)
			{
				AlertDialog.Builder dialog = new AlertDialog.Builder(this);
				dialog.setTitle(getString(R.string.support_dialog_title));
				dialog.setIcon(getResources().getDrawable(R.drawable.ic_launcher));
				dialog.setMessage(R.string.support_dialog_text);

				dialog.setPositiveButton(getString(R.string.support_dialog_yes), new OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						startActivity(new Intent(MainMenuActivity.this, AboutActivity.class));
						try{
							dialog.dismiss();
						}
						catch(Exception e) {
						}
					}
				});
				dialog.setNeutralButton(getString(R.string.support_dialog_no), new OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						try{
							dialog.dismiss();
						}
						catch(Exception e) {
						}
					}
				});
				dialog.setNegativeButton(getString(R.string.support_dialog_neveragain), new OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						SharedPreferences settings = getSharedPreferences(Constants.PREFSNAME, MODE_PRIVATE);
						SharedPreferences.Editor ed = settings.edit();
						ed.putBoolean(Constants.PREFS_KEY_SUPPORTDLG, false);
						ed.commit();

						try{
							dialog.dismiss();
						}
						catch(Exception e) {
						}
					}
				});

				dialog.show();
			}
		}
		SharedPreferences.Editor ed = settings.edit();
		ed.putInt(Constants.PREFS_KEY_MAIN_MENU_STARTS, settings.getInt(Constants.PREFS_KEY_MAIN_MENU_STARTS, 0)+1);
		ed.apply();


		/*
		 * show changelog if version changed
		 */
		try {
			//current version
			PackageInfo packageInfo = getPackageManager().getPackageInfo(getPackageName(), 0);
			int versionCode = packageInfo.versionCode;

			int lastVersionCode = settings.getInt("lastVersionCode", 0);

			if(lastVersionCode < versionCode) {
				SharedPreferences.Editor editor = settings.edit();
				editor.putInt("lastVersionCode", versionCode);
				editor.commit();

				AlertDialog.Builder builder = new AlertDialog.Builder(this);
				builder.setTitle(getString(R.string.changelog_dialog_title));
				builder.setIcon(getResources().getDrawable(R.drawable.ic_launcher));
				builder.setMessage(Html.fromHtml(getString(R.string.changelog_dialog_text)));
				builder.setPositiveButton(getString(android.R.string.ok), new OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						try{
							dialog.dismiss();
						}
						catch(Exception e) {
						}
					}
				});

				AlertDialog dialog = builder.create();
				dialog.show();
				TextView msgTxt = (TextView) dialog.findViewById(android.R.id.message);
				msgTxt.setMovementMethod(LinkMovementMethod.getInstance());
			}
		} catch (NameNotFoundException e) {
			Log.w(TAG, "Unable to get version code. Will not show changelog", e);
		}


	}

	protected void onDestroy() {

		Log.d(TAG, "onDestroy()");

		super.onDestroy();

		unbindFromMDNSService();
	}

	@Override
	public void onBackPressed() {
		super.onBackPressed();
		stopService(new Intent(Intent.ACTION_VIEW, null, this, MDNSService.class));
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onCreateOptionsMenu(android.view.Menu)
	 */
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getMenuInflater().inflate(R.menu.mainmenuactivitymenu,menu);

		menu.findItem(R.id.itemMDNSRestart).setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS);
		menu.findItem(R.id.itemImportExport).setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS);
		menu.findItem(R.id.itemOpenDoc).setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS);

		return true;
	}


	/* (non-Javadoc)
	 * @see android.app.Activity#onOptionsItemSelected(android.view.MenuItem)
	 */
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId())
		{
		case R.id.itemMDNSRestart :
			serverlist.removeAllViews();
			findViewById(R.id.discovered_servers_waitwheel).setVisibility(View.VISIBLE);

			try {
				boundMDNSService.restart();
			}
			catch(NullPointerException e) {
			}
			break;
		case R.id.itemImportExport :
			new ImportExportDialog().show(getSupportFragmentManager(), "importexport");
			break;
		case R.id.itemOpenDoc :
			Intent intent = new Intent (this, AboutActivity.class);
			this.startActivity(intent);
			break;
		}
		return true;
	}




	protected void onStart() {
		super.onStart();
		updateBookmarkView();
	}


	void updateBookmarkView() {
		List<ConnectionBean> bookmarked_connections = database.getConnectionDao().getAll();

		Collections.sort(bookmarked_connections);

		Log.d(TAG, "updateBookMarkView()");

//		int connectionIndex=0;
//		if ( bookmarked_connections.size()>1)
//		{
//			MostRecentBean mostRecent = getMostRecent(database.getReadableDatabase());
//			if (mostRecent != null)
//			{
//				for ( int i=1; i<bookmarked_connections.size(); ++i)
//				{
//					if (bookmarked_connections.get(i).get_Id() == mostRecent.getConnectionId())
//					{
//						connectionIndex=i;
//						break;
//					}
//				}
//			}
//		}

		// update bookmarks list
		bookmarkslist.removeAllViews();
		LayoutInflater vi = (LayoutInflater)getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		for(int i=0; i < bookmarked_connections.size(); ++i)
		{
			final ConnectionBean conn = bookmarked_connections.get(i);
			View v = vi.inflate(R.layout.bookmarks_list_item, null);

			Log.d(TAG, "Displaying bookmark: " + conn.toString());

			// name
			TextView name = (TextView) v.findViewById(R.id.bookmark_name);
			name.setText(conn.nickname);
			name.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View view) {
					Log.d(TAG, "Starting bookmarked connection " + conn.toString());
					Intent intent = new Intent(MainMenuActivity.this, VncCanvasActivity.class);
					intent.putExtra(Constants.CONNECTION , conn);
					startActivity(intent);
				}
			});

			// button
			ImageButton button = (ImageButton) v.findViewById(R.id.bookmark_edit_button);
			button.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View view) {

					final CharSequence[] items = {
							getString(R.string.save_as_copy),
							getString(R.string.delete_connection),
							getString(R.string.edit)
					};

					AlertDialog.Builder builder = new AlertDialog.Builder(MainMenuActivity.this);
					builder.setItems(items, new DialogInterface.OnClickListener() {
					    public void onClick(DialogInterface dialog, int item) {
					    	switch(item) {

					    	case 0: // save as copy
					    		/* the ConnectionBean objects are transient and generated
					    		   anew with each call to this function
					    		 */
					    		conn.nickname = "Copy of " + conn.nickname;
					    		conn.id = 0L; // this saves a new one in the DB!
								saveBookmark(conn);
								// update
								updateBookmarkView();
					    		break;

					    	case 1: // delete
					    		AlertDialog.Builder builder = new AlertDialog.Builder(MainMenuActivity.this);
								builder.setMessage(getString(R.string.bookmark_delete_question))
								.setCancelable(false)
								.setPositiveButton(getString(android.R.string.yes), new DialogInterface.OnClickListener() {
									public void onClick(DialogInterface dialog, int id) {
										Log.d(TAG, "Deleting bookmark " + conn.id);
										// delete from DB
										database.getConnectionDao().delete(conn);
										// update
										updateBookmarkView();
									}
								})
								.setNegativeButton(getString(android.R.string.no), new DialogInterface.OnClickListener() {
									public void onClick(DialogInterface dialog, int id) {
										dialog.cancel();
									}
								});
								AlertDialog saveornot = builder.create();
								saveornot.show();
					    		break;

					    	case 2: // edit
								Log.d(TAG, "Editing bookmark " + conn.id);
					    		Intent intent = new Intent(MainMenuActivity.this, ConnectionEditActivity.class);
					    		intent.putExtra(Constants.CONNECTION, conn.id);
					    		startActivity(intent);
					    		break;

					    	}
					    }
					});
					AlertDialog chooser = builder.create();
					chooser.show();
				}
			});

			bookmarkslist.addView(v);
		}


	}


	VncDatabase getDatabaseHelper()
	{
		return database;
	}


	private void saveBookmark(ConnectionBean conn) 	{
		try {
			Log.d(TAG, "Saving bookmark for conn " + conn.toString());
			database.getConnectionDao().save(conn);
		}
		catch(Exception e) {
			Log.e(TAG, "Error saving bookmark: " + e.getMessage());
		}
	}



	void startMDNSService() {

		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
			if (ProcessLifecycleOwner.get().getLifecycle().getCurrentState().isAtLeast(Lifecycle.State.STARTED)) {
				Intent serviceIntent = new Intent(Intent.ACTION_VIEW, null, this, MDNSService.class);
				try {
					this.startService(serviceIntent);
				}
				catch(IllegalStateException e) {
					// Can still happen on Android 9 due to https://issuetracker.google.com/issues/113122354
					// At least, don't crash - user's can restart the scanner manually anyway.
				}
			} else {
				ProcessLifecycleOwner.get().getLifecycle().addObserver(this);
			}
		} else {
			Intent serviceIntent = new Intent(Intent.ACTION_VIEW, null, this, MDNSService.class);
			this.startService(serviceIntent);
		}
	}

	@OnLifecycleEvent(Lifecycle.Event.ON_START)
	void onEnterForeground() {
		startMDNSService();
		ProcessLifecycleOwner.get().getLifecycle().removeObserver(this);
	}

	void bindToMDNSService(Intent serviceIntent) {
		unbindFromMDNSService();
		connection_MDNSService = new MDNSServiceConnection();
		if (!this.bindService(serviceIntent, connection_MDNSService, Context.BIND_AUTO_CREATE))
			Toast.makeText(this, "Could not bind to MDNSService!", Toast.LENGTH_LONG).show();
	}

	void unbindFromMDNSService()
	{
		if(connection_MDNSService != null)
		{
			this.unbindService(connection_MDNSService);
			connection_MDNSService = null;
			try {
				boundMDNSService.registerCallback(null);
				boundMDNSService = null;
			} catch (NullPointerException e) {
				// was already null
			}
		}
	}

	@Override
	public void onMDNSnotify(final String conn_name, final ConnectionBean conn, final Hashtable<String,ConnectionBean> connections_discovered ) {
		handler.postDelayed(new Runnable() {
			public void run() {

				String msg;
				if(conn != null)
					msg = getString(R.string.server_found) + ": " + conn_name;
				else
					msg = getString(R.string.server_removed) + ": " + conn_name;

				// only show Toasts if activity is in foreground
				if(getLifecycle().getCurrentState().isAtLeast(Lifecycle.State.RESUMED))
					Toast.makeText(getApplicationContext(), msg , Toast.LENGTH_SHORT).show();

				// update
				LayoutInflater vi = (LayoutInflater)getSystemService(Context.LAYOUT_INFLATER_SERVICE);

				/*
				 * we don't care whether this is a conn add or removal (conn!=null or conn==null),
				 * we always rebuild the whole view for the sake of code simplicity
				 */
				serverlist.removeAllViews();
				// refill
				synchronized (connections_discovered) {
					for(final ConnectionBean c : connections_discovered.values()) {
						// the list item
						View v = vi.inflate(R.layout.discovered_servers_list_item, null);

						// name part of list item
						TextView name = (TextView) v.findViewById(R.id.discovered_server_name);
						name.setText(c.nickname);
						name.setOnClickListener(new View.OnClickListener() {
							@Override
							public void onClick(View view) {
								Log.d(TAG, "Starting discovered connection " + c.toString());
								Intent intent = new Intent(MainMenuActivity.this, VncCanvasActivity.class);
								intent.putExtra(Constants.CONNECTION , c);
								startActivity(intent);
							}
						});

						// button part of list item
						ImageButton button = (ImageButton) v.findViewById(R.id.discovered_server_save_button);
						button.setOnClickListener(new View.OnClickListener() {
							@Override
							public void onClick(View view) {
								AlertDialog.Builder builder = new AlertDialog.Builder(MainMenuActivity.this);
								builder.setMessage(getString(R.string.bookmark_save_question))
								.setCancelable(false)
								.setPositiveButton(getString(android.R.string.yes), new DialogInterface.OnClickListener() {
									public void onClick(DialogInterface dialog, int id) {
										Log.d(TAG, "Bookmarking connection " + c.id);
										// save bookmark
										saveBookmark(c);
										// set as 'new' again. makes this ConnectionBean saveable again
										c.id = 0;
										// and update view
										updateBookmarkView();
									}
								})
								.setNegativeButton(getString(android.R.string.no), new DialogInterface.OnClickListener() {
									public void onClick(DialogInterface dialog, int id) {
										dialog.cancel();
									}
								});
								AlertDialog saveornot = builder.create();
								saveornot.show();

							}
						});

						serverlist.addView(v);
					}
				}



			}
		}, 1);

	}

	@Override
	public void onMDNSstartupCompleted(boolean wasSuccessful) {
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				try {
					findViewById(R.id.discovered_servers_waitwheel).setVisibility(View.GONE);
				}
				catch(Exception e) {
					//unused
				}
			}
		});
	}
}
