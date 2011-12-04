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
import android.app.ActivityManager.MemoryInfo;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.database.sqlite.SQLiteDatabase;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Hashtable;

import javax.jmdns.JmDNS;
import javax.jmdns.ServiceEvent;
import javax.jmdns.ServiceListener;



public class MainMenuActivity extends Activity {
	
	private static final String TAG = "MainActivity";
	
	private EditText ipText;
	private EditText portText;
	private EditText passwordText;
	private Button goButton;
	private TextView repeaterText;
	//private RadioGroup groupForceFullScreen;
	//private Spinner colorSpinner;
	private LinearLayout serverlist;
	private LinearLayout bookmarkslist;

	private VncDatabase database;
	private ConnectionBean conn_new;
	private EditText textUsername;
	private CheckBox checkboxKeepPassword;
	
	// service discovery stuff
	private android.net.wifi.WifiManager.MulticastLock multicastLock;
	private android.os.Handler handler = new android.os.Handler();
	private String mdnstype = "_rfb._tcp.local.";
	private JmDNS jmdns = null;
	private ServiceListener listener = null;
	private Hashtable<String,ConnectionBean> connections_discovered = new Hashtable<String,ConnectionBean> (); 

	
	@Override
	public void onCreate(Bundle icicle) {

		super.onCreate(icicle);
		setContentView(R.layout.main);
		
		// get package debug flag and set it 
		Utils.DEBUG(this);
		
		
		ipText = (EditText) findViewById(R.id.textIP);
		portText = (EditText) findViewById(R.id.textPORT);
		passwordText = (EditText) findViewById(R.id.textPASSWORD);
		textUsername = (EditText) findViewById(R.id.textUsername);
		goButton = (Button) findViewById(R.id.buttonGO);
		
		serverlist = (LinearLayout) findViewById(R.id.discovered_servers_list);
		bookmarkslist = (LinearLayout) findViewById(R.id.bookmarks_list);

		
		//colorSpinner = (Spinner)findViewById(R.id.colorformat);
		COLORMODEL[] models=COLORMODEL.values();
		ArrayAdapter<COLORMODEL> colorSpinnerAdapter = new ArrayAdapter<COLORMODEL>(this, android.R.layout.simple_spinner_item, models);
		//groupForceFullScreen = (RadioGroup)findViewById(R.id.groupForceFullScreen);
		checkboxKeepPassword = (CheckBox)findViewById(R.id.checkboxKeepPassword);
		//colorSpinner.setAdapter(colorSpinnerAdapter);
		//colorSpinner.setSelection(0);
		
		
		
//		spinnerConnection = (Spinner)findViewById(R.id.spinnerConnection);
//		spinnerConnection.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
//			@Override
//			public void onItemSelected(AdapterView<?> ad, View view, int itemIndex, long id) {
//				conn_new = (ConnectionBean)ad.getSelectedItem();
//				updateViewFromSelected();
//			}
//			@Override
//			public void onNothingSelected(AdapterView<?> ad) {
//				conn_new = null;
//			}
//		});
//		spinnerConnection.setOnItemLongClickListener(new AdapterView.OnItemLongClickListener() {
//
//			/* (non-Javadoc)
//			 * @see android.widget.AdapterView.OnItemLongClickListener#onItemLongClick(android.widget.AdapterView, android.view.View, int, long)
//			 */
//			@Override
//			public boolean onItemLongClick(AdapterView<?> arg0, View arg1,
//					int arg2, long arg3) {
//				spinnerConnection.setSelection(arg2);
//				conn_new = (ConnectionBean)spinnerConnection.getItemAtPosition(arg2);
//				canvasStart();
//				return true;
//			}
//			
//		});
//		
		
		
		
		
		repeaterText = (TextView)findViewById(R.id.textRepeaterId);
		goButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View view) {
				vnc();
			}
		});
		
		database = new VncDatabase(this);
	}
	
	protected void onDestroy() {
		super.onDestroy();

		database.close();
		mDNSstop();
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
		arriveOnPage();
		mDNSstart();
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
	
	void arriveOnPage() {
		ArrayList<ConnectionBean> bookmarked_connections=new ArrayList<ConnectionBean>();
		ConnectionBean.getAll(database.getReadableDatabase(), ConnectionBean.GEN_TABLE_NAME, bookmarked_connections, ConnectionBean.newInstance);
		Collections.sort(bookmarked_connections);
		
		Log.d(TAG, "main menu arriveonpage()");
		
		int connectionIndex=0;
		if ( bookmarked_connections.size()>1)
		{
			MostRecentBean mostRecent = getMostRecent(database.getReadableDatabase());
			if (mostRecent != null)
			{
				for ( int i=1; i<bookmarked_connections.size(); ++i)
				{
					if (bookmarked_connections.get(i).get_Id() == mostRecent.getConnectionId())
					{
						connectionIndex=i;
						break;
					}
				}
			}
		}

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
								arriveOnPage();
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
										arriveOnPage();
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
		
		try {
			conn_new=bookmarked_connections.get(connectionIndex);
		}
		catch (IndexOutOfBoundsException e) {
			conn_new = null;
		}
		
	}
	
	protected void onStop() {
		super.onStop();
		if ( conn_new == null ) {
			return;
		}
		updateNewConnFromView();
		conn_new.save(database.getWritableDatabase());
	}
	
	VncDatabase getDatabaseHelper()
	{
		return database;
	}
	
	
	
	private void saveAndWriteRecent()
	{
		SQLiteDatabase db = database.getWritableDatabase();
		db.beginTransaction();
		try
		{
			conn_new.save(db);
			MostRecentBean mostRecent = getMostRecent(db);
			if (mostRecent == null)
			{
				mostRecent = new MostRecentBean();
				mostRecent.setConnectionId(conn_new.get_Id());
				mostRecent.Gen_insert(db);
			}
			else
			{
				mostRecent.setConnectionId(conn_new.get_Id());
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
			Log.d(TAG, "Saving bookmark for conn " + conn.get_Id());
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
	

	private void updateNewConnFromView() {
	
		conn_new.setAddress(ipText.getText().toString());
		try
		{
			conn_new.setPort(Integer.parseInt(portText.getText().toString()));
		}
		catch (NumberFormatException nfe)
		{
			
		}
		conn_new.setUserName(textUsername.getText().toString());
		//conn_new.setForceFull(groupForceFullScreen.getCheckedRadioButtonId()==R.id.radioForceFullScreenAuto ? BitmapImplHint.AUTO : (groupForceFullScreen.getCheckedRadioButtonId()==R.id.radioForceFullScreenOn ? BitmapImplHint.FULL : BitmapImplHint.TILE));
		conn_new.setPassword(passwordText.getText().toString());
		conn_new.setKeepPassword(checkboxKeepPassword.isChecked());
		conn_new.setUseLocalCursor(true); // always enable
		//conn_new.setColorModel(((COLORMODEL)colorSpinner.getbookmarkItem()).nameString());
		if (repeaterText.getText().length() > 0)
		{
			conn_new.setRepeaterId(repeaterText.getText().toString());
			conn_new.setUseRepeater(true);
		}
		else
		{			
			conn_new.setUseRepeater(false);
		}
	}
	
	
	private void vnc() {
		if(conn_new == null)
			return;
		updateNewConnFromView();
		saveAndWriteRecent();
		Log.d(TAG, "Starting NEW connection " + conn_new.toString());
		Intent intent = new Intent(this, VncCanvasActivity.class);
		intent.putExtra(VncConstants.CONNECTION,conn_new.Gen_getValues());
		startActivity(intent);
	}
	
	
	

	private void mDNSstart()
	{
		Log.d(TAG, "starting MDNS");
		
		if(jmdns != null) {
			Log.d(TAG, "MDNS already running, bailing out");
			return;
		}
		
		android.net.wifi.WifiManager wifi = (android.net.wifi.WifiManager) getSystemService(android.content.Context.WIFI_SERVICE);
		multicastLock = wifi.createMulticastLock("mylockthereturn");
		multicastLock.setReferenceCounted(true);
		multicastLock.acquire();
		try {
			jmdns = JmDNS.create();
			jmdns.addServiceListener(mdnstype, listener = new ServiceListener() {

				@Override
				public void serviceResolved(ServiceEvent ev) {
					ConnectionBean c = new ConnectionBean();
					c.set_Id(0); // new!
					c.setNickname(ev.getName());
					c.setAddress(ev.getInfo().getInetAddresses()[0].toString().replace('/', ' ').trim());
					c.setPort(ev.getInfo().getPort());
					
					connections_discovered.put(ev.getInfo().getQualifiedName(), c);
				
					Log.d(TAG, "discovered server :" + ev.getName() 
								+ ", now " + connections_discovered.size());
					
					mDNSnotify(getString(R.string.server_found) + ": " + ev.getName(), c);
				}

				@Override
				public void serviceRemoved(ServiceEvent ev) {
					connections_discovered.remove(ev.getInfo().getQualifiedName());
					
					Log.d(TAG, "server gone:" + ev.getName() 
							+ ", now " + connections_discovered.size());
					
					mDNSnotify(getString(R.string.server_removed) + ": " + ev.getName(), null);
				}

				@Override
				public void serviceAdded(ServiceEvent event) {
					// Required to force serviceResolved to be called again (after the first search)
					jmdns.requestServiceInfo(event.getType(), event.getName(), 1);
				}
			});

		} catch (IOException e) {
			e.printStackTrace();
			return;
		}
	}

	private void mDNSstop()
	{
		Log.d(TAG, "stopping MDNS");
		if (jmdns != null) {
			if (listener != null) {
				jmdns.removeServiceListener(mdnstype, listener);
				listener = null;
			}
			try {
				jmdns.close();
			} catch (IOException e) {
				e.printStackTrace();
			}
			jmdns = null;
		}

		multicastLock.release();
		
		connections_discovered.clear();
		((LinearLayout)findViewById(R.id.discovered_servers_list)).removeAllViews();
		
		Log.d(TAG, "stopping MDNS done");
	}

	// do the GUI stuff in Runnable posted to main thread handler
	private void mDNSnotify(final String msg, final ConnectionBean conn) {
		handler.postDelayed(new Runnable() {
			public void run() {

				Toast.makeText(MainMenuActivity.this, msg , Toast.LENGTH_SHORT).show();

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
										arriveOnPage();
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
