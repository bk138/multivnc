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

package com.coboltforge.dontmind.multivnc;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.database.sqlite.SQLiteDatabase;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Hashtable;




public class MainMenuActivity extends Activity implements ImDNSNotify {
	
	private static final String TAG = "MainMenuActivity";
	
	private EditText ipText;
	private EditText portText;
	private EditText passwordText;
	private TextView repeaterText;
	//private RadioGroup groupForceFullScreen;
	//private Spinner colorSpinner;
	private LinearLayout serverlist;
	private LinearLayout bookmarkslist;

	private VncDatabase database;
	private EditText textUsername;
	private CheckBox checkboxKeepPassword;
	
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
			
			Log.d(TAG, "bound to MDNSService " + boundMDNSService);
		}

		public void onServiceDisconnected(ComponentName className) {
			// This is called when the connection with the service has been
			// unexpectedly disconnected -- that is, its process crashed.
			// Because it is running in our same process, we should never
			// see this happen.
			boundMDNSService = null;
		}
	};
	
	
	
	@Override
	public void onCreate(Bundle icicle) {

		super.onCreate(icicle);
		setContentView(R.layout.main);
		
		// get package debug flag and set it 
		Utils.DEBUG(this);
		
		// start the MDNS service
		startMDNSService();
		// and (re-)bind to MDNS service
		bindToMDNSService(new Intent(this, MDNSService.class));
		
		ipText = (EditText) findViewById(R.id.textIP);
		portText = (EditText) findViewById(R.id.textPORT);
		passwordText = (EditText) findViewById(R.id.textPASSWORD);
		textUsername = (EditText) findViewById(R.id.textUsername);
		
		serverlist = (LinearLayout) findViewById(R.id.discovered_servers_list);
		bookmarkslist = (LinearLayout) findViewById(R.id.bookmarks_list);

		
		//colorSpinner = (Spinner)findViewById(R.id.colorformat);
		//COLORMODEL[] models=COLORMODEL.values();
		//ArrayAdapter<COLORMODEL> colorSpinnerAdapter = new ArrayAdapter<COLORMODEL>(this, android.R.layout.simple_spinner_item, models);
		//groupForceFullScreen = (RadioGroup)findViewById(R.id.groupForceFullScreen);
		checkboxKeepPassword = (CheckBox)findViewById(R.id.checkboxKeepPassword);
		//colorSpinner.setAdapter(colorSpinnerAdapter);
		//colorSpinner.setSelection(0);
		
		
		repeaterText = (TextView)findViewById(R.id.textRepeaterId);
		
		Button goButton = (Button) findViewById(R.id.buttonGO);
		goButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View view) {
				ConnectionBean conn = makeNewConnFromView();
				if(conn == null)
					return;
				writeRecent(conn);
				Log.d(TAG, "Starting NEW connection " + conn.toString());
				Intent intent = new Intent(MainMenuActivity.this, VncCanvasActivity.class);
				intent.putExtra(VncConstants.CONNECTION,conn.Gen_getValues());
				startActivity(intent);
			}
		});
		
		Button saveBookmarkButton = (Button) findViewById(R.id.buttonSaveBookmark);
		saveBookmarkButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				
				final ConnectionBean conn = makeNewConnFromView();
				if(conn == null)
					return;

				final EditText input = new EditText(MainMenuActivity.this);

				new AlertDialog.Builder(MainMenuActivity.this)
				.setMessage(getString(R.string.enterbookmarkname))
				.setView(input)
				.setPositiveButton(getString(android.R.string.ok), new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int whichButton) {
						String name = input.getText().toString();
						if(name.length() == 0) 
							name = conn.getAddress() + ":" + conn.getPort(); 
						conn.setNickname(name);
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
		
		database = new VncDatabase(this);
	}
	
	protected void onDestroy() {
		super.onDestroy();

		database.close();
		
		unbindFromMDNSService();
	}
	
	/* (non-Javadoc)
	 * @see android.app.Activity#onCreateDialog(int)
	 */
	@Override
	protected Dialog onCreateDialog(int id) {
		if (id == R.layout.importexport)
			return new ImportExportDialog(this);
		
		return null;
	}
	
	

	/* (non-Javadoc)
	 * @see android.app.Activity#onCreateOptionsMenu(android.view.Menu)
	 */
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getMenuInflater().inflate(R.menu.androidvncmenu,menu);
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
			try {
				boundMDNSService.restart();
			}
			catch(NullPointerException e) {
			}
			break;
		case R.id.itemImportExport :
			showDialog(R.layout.importexport);
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
	
	
	
	/**
	 * Return the object representing the app global state in the database, or null
	 * if the object hasn't been set up yet
	 * @param db App's database -- only needs to be readable
	 * @return Object representing the single persistent instance of MostRecentBean, which
	 * is the app's global state
	 */
	static MostRecentBean getMostRecent(SQLiteDatabase db)
	{
		ArrayList<MostRecentBean> recents = new ArrayList<MostRecentBean>(1);
		MostRecentBean.getAll(db, MostRecentBean.GEN_TABLE_NAME, recents, MostRecentBean.GEN_NEW);
		if (recents.size() == 0)
			return null;
		return recents.get(0);
	}
	
	void updateBookmarkView() {
		ArrayList<ConnectionBean> bookmarked_connections=new ArrayList<ConnectionBean>();
		ConnectionBean.getAll(database.getReadableDatabase(), ConnectionBean.GEN_TABLE_NAME, bookmarked_connections, ConnectionBean.newInstance);
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
			name.setText(conn.getNickname());
			name.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View view) {
					Log.d(TAG, "Starting bookmarked connection " + conn.toString());
					writeRecent(conn);
					Intent intent = new Intent(MainMenuActivity.this, VncCanvasActivity.class);
					intent.putExtra(VncConstants.CONNECTION , conn.Gen_getValues());
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
					    		conn.setNickname("Copy of " + conn.getNickname());
					    		conn.set_Id(0); // this saves a new one in the DB!
								saveBookmark(conn);
								// update
								updateBookmarkView();
					    		break;
					    		
					    	case 1: // delete
					    		AlertDialog.Builder builder = new AlertDialog.Builder(MainMenuActivity.this);
								builder.setMessage(getString(R.string.bookmark_delete_question))
								.setCancelable(false)
								.setPositiveButton(getString(R.string.yes), new DialogInterface.OnClickListener() {
									public void onClick(DialogInterface dialog, int id) {
										Log.d(TAG, "Deleting bookmark " + conn.get_Id());
										// delete from DB
										conn.Gen_delete(database.getWritableDatabase());
										// update
										updateBookmarkView();
									}
								})
								.setNegativeButton(getString(R.string.no), new DialogInterface.OnClickListener() {
									public void onClick(DialogInterface dialog, int id) {
										dialog.cancel();
									}
								});
								AlertDialog saveornot = builder.create();
								saveornot.show();
					    		break;
					    		
					    	case 2: // edit
								Log.d(TAG, "Editing bookmark " + conn.get_Id());
					    		Intent intent = new Intent(MainMenuActivity.this, EditBookmarkActivity.class);
					    		intent.putExtra(VncConstants.CONNECTION, conn.get_Id());
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
	
	
	
	private void writeRecent(ConnectionBean conn)
	{
		SQLiteDatabase db = database.getWritableDatabase();
		db.beginTransaction();
		try
		{
			MostRecentBean mostRecent = getMostRecent(db);
			if (mostRecent == null)
			{
				mostRecent = new MostRecentBean();
				mostRecent.setConnectionId(conn.get_Id());
				mostRecent.Gen_insert(db);
			}
			else
			{
				mostRecent.setConnectionId(conn.get_Id());
				mostRecent.Gen_update(db);
			}
			db.setTransactionSuccessful();
		}
		finally
		{
			db.endTransaction();
		}
	}
	
	private void saveBookmark(ConnectionBean conn) 	{
		SQLiteDatabase db = database.getWritableDatabase();
		db.beginTransaction();
		try {
			Log.d(TAG, "Saving bookmark for conn " + conn.toString());
			conn.save(db);
			db.setTransactionSuccessful();
		}
		catch(Exception e) {
			Log.e(TAG, "Error saving bookmark: " + e.getMessage());
		}
		finally {
			db.endTransaction();
		}
	}
	

	private ConnectionBean makeNewConnFromView() {
	
		ConnectionBean conn = new ConnectionBean();
		
		conn.setAddress(ipText.getText().toString());
		
		if(conn.getAddress().length() == 0)
			return null;
		
		conn.set_Id(0); // is new!!
		
		try {
			conn.setPort(Integer.parseInt(portText.getText().toString()));
		}
		catch (NumberFormatException nfe) {			
		}
		conn.setUserName(textUsername.getText().toString());
		//conn_new.setForceFull(groupForceFullScreen.getCheckedRadioButtonId()==R.id.radioForceFullScreenAuto ? BitmapImplHint.AUTO : (groupForceFullScreen.getCheckedRadioButtonId()==R.id.radioForceFullScreenOn ? BitmapImplHint.FULL : BitmapImplHint.TILE));
		conn.setPassword(passwordText.getText().toString());
		conn.setKeepPassword(checkboxKeepPassword.isChecked());
		conn.setUseLocalCursor(true); // always enable
		//conn_new.setColorModel(((COLORMODEL)colorSpinner.getbookmarkItem()).nameString());
		if (repeaterText.getText().length() > 0)
		{
			conn.setRepeaterId(repeaterText.getText().toString());
			conn.setUseRepeater(true);
		}
		else
		{			
			conn.setUseRepeater(false);
		}
		
		return conn;
	}
	
	


	void startMDNSService() {
		Intent serviceIntent = new Intent(Intent.ACTION_VIEW, null, this, MDNSService.class);
		this.startService(serviceIntent);
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
			boundMDNSService = null;
		}
	}

	@Override
	public void mDNSnotify(final String conn_name, final ConnectionBean conn, final Hashtable<String,ConnectionBean> connections_discovered ) {
		handler.postDelayed(new Runnable() {
			public void run() {

				String msg;
				if(conn != null)
					msg = getString(R.string.server_found) + ": " + conn_name;
				else
					msg = getString(R.string.server_removed) + ": " + conn_name;
				
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
						name.setText(c.getNickname());
						name.setOnClickListener(new View.OnClickListener() {
							@Override
							public void onClick(View view) {
								Log.d(TAG, "Starting discovered connection " + conn.toString());
								writeRecent(conn);
								Intent intent = new Intent(MainMenuActivity.this, VncCanvasActivity.class);
								intent.putExtra(VncConstants.CONNECTION , c.Gen_getValues());
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
								.setPositiveButton(getString(R.string.yes), new DialogInterface.OnClickListener() {
									public void onClick(DialogInterface dialog, int id) {
										Log.d(TAG, "Bookmarking connection " + c.get_Id());
										// save bookmark
										saveBookmark(c);
										// set as 'new' again. makes this ConnectionBean saveable again
										c.set_Id(0);
										// and update view
										updateBookmarkView();
									}
								})
								.setNegativeButton(getString(R.string.no), new DialogInterface.OnClickListener() {
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
}
