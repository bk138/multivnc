package com.coboltforge.dontmind.multivnc;

import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

import androidx.appcompat.app.AppCompatActivity;

/**
 * Bookmark editing activity.
 */
public class ConnectionEditActivity extends AppCompatActivity {
	
	private static final String TAG = "EditBookmarkActivity";
	private VncDatabase database;
	private ConnectionBean bookmark = new ConnectionBean();

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setContentView(R.layout.connection_edit_activity);

		database = VncDatabase.getInstance(this);

		// default return value
		setResult(RESULT_CANCELED);


		// read connection from DB
		Intent intent = getIntent();
		long connID = intent.getLongExtra(Constants.CONNECTION, 0);
		bookmark = database.getConnectionDao().get(connID);
		if (bookmark != null)
		{
			Log.d(TAG, "Successfully read connection " + connID + " from database");

			ConnectionEditFragment editor = (ConnectionEditFragment) getSupportFragmentManager().findFragmentById(R.id.connectionEditFragment);
			if (editor != null) {
				editor.setConnection(bookmark);
			}
		}
		else 
		{
			Log.e(TAG, "Error reading connection " + connID + " from database!");
		}

		Button saveButton = (Button) findViewById(R.id.buttonSaveBookmark);
		saveButton.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {

				ConnectionEditFragment editor = (ConnectionEditFragment) getSupportFragmentManager().findFragmentById(R.id.connectionEditFragment);
				bookmark = editor != null ? editor.getConnection() : null;
				saveBookmark(bookmark);
				
				setResult(RESULT_OK);
				finish();
			}
		});
		
		
		Button cancelButton = (Button) findViewById(R.id.buttonCancelBookmark);
		cancelButton.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				finish();
			}
		});
		
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
	    switch (item.getItemId()) {
	        case android.R.id.home:
	            // app icon in action bar clicked; go home
	            Intent intent = new Intent(this, MainMenuActivity.class);
	            intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
	            startActivity(intent);
	            return true;
	        default:
	            return super.onOptionsItemSelected(item);
	    }
	}


	private void saveBookmark(ConnectionBean conn) 	{
		try {
			Log.d(TAG, "Saving bookmark for conn " + conn.id);
			database.getConnectionDao().save(conn);
		}
		catch(Exception e) {
			Log.e(TAG, "Error saving bookmark: " + e.getMessage());
		}
	}
}