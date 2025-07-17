package com.coboltforge.dontmind.multivnc.ui;

import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.widget.Button;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;

import com.coboltforge.dontmind.multivnc.Constants;
import com.coboltforge.dontmind.multivnc.R;
import com.coboltforge.dontmind.multivnc.db.ConnectionBean;
import com.coboltforge.dontmind.multivnc.db.VncDatabase;

import java.util.Objects;

/**
 * Bookmark editing activity.
 */
public class ConnectionEditActivity extends AppCompatActivity {
	
	private static final String TAG = "EditBookmarkActivity";

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		// On Android 15 and later, calling enableEdgeToEdge ensures system bar icon colors update
		// when the device theme changes. Because calling it on pre-Android 15 has the side effect of
		// enabling EdgeToEdge there as well, we only use it on Android 15 and later.
		if (Build.VERSION.SDK_INT >= 35) {
			EdgeToEdge.enable(this);
		}

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