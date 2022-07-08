package com.coboltforge.dontmind.multivnc;

import android.os.Bundle;
import android.util.Log;
import android.widget.Button;
import androidx.appcompat.app.AppCompatActivity;

import java.util.Objects;

/**
 * Bookmark editing activity.
 */
public class ConnectionEditActivity extends AppCompatActivity {
	
	private static final String TAG = "EditBookmarkActivity";

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setContentView(R.layout.connection_edit_activity);

		VncDatabase database = VncDatabase.getInstance(this);

		// default return value
		setResult(RESULT_CANCELED);

		// read connection from DB
		long connID = getIntent().getLongExtra(Constants.CONNECTION, 0);
		ConnectionBean bookmark = database.getConnectionDao().get(connID);
		if (bookmark != null) {
			Log.d(TAG, "Successfully read connection " + connID + " from database");
			ConnectionEditFragment editor = (ConnectionEditFragment) getSupportFragmentManager().findFragmentById(R.id.connectionEditFragment);
			if (editor != null) {
				editor.setConnection(bookmark);
			}
		}
		else {
			Log.e(TAG, "Error reading connection " + connID + " from database!");
		}

		Button saveButton = (Button) findViewById(R.id.buttonSaveBookmark);
		saveButton.setOnClickListener(v -> {
			try {
				ConnectionEditFragment editor = (ConnectionEditFragment) getSupportFragmentManager().findFragmentById(R.id.connectionEditFragment);
				ConnectionBean conn = Objects.requireNonNull(editor).getConnection();
				Log.d(TAG, "Saving bookmark for conn " + conn.id);
				database.getConnectionDao().save(conn);
				setResult(RESULT_OK);
			}
			catch(Exception e) {
				Log.e(TAG, "Error saving bookmark: " + e.getMessage());
			}
			finish();
		});
		
		Button cancelButton = (Button) findViewById(R.id.buttonCancelBookmark);
		cancelButton.setOnClickListener(v -> finish());
	}

}