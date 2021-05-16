/*
 * Copyright (c) 2010 Michael A. MacDonald
 * Copyright (c) 2011-2019 Christian Beier
 */
package com.coboltforge.dontmind.multivnc;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

import com.antlersoft.android.contentxml.SqliteElement;
import com.antlersoft.android.contentxml.SqliteElement.ReplaceStrategy;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.Writer;

import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;

import org.xml.sax.SAXException;

/**
 * @author Michael A. MacDonald
 *
 */
public class ImportExportActivity extends Activity {

	private final static String TAG = "ImportExportActivity";

	private EditText _textLoadUrl;
	private EditText _textSaveUrl;
	private Button mButtonExport;
	private Button mButtonImport;
	private DbOpener dbOpener;
	private VncDatabase vncDatabase;

	@SuppressLint("StaticFieldLeak") // this is not long-running
	private class ImportFromURLAsyncTask extends AsyncTask<String, Void, Exception> {

		@Override
		protected Exception doInBackground(String... urls) {
			try {
				URLConnection connection = new URL(urls[0]).openConnection();
				connection.connect();
				InputStreamReader reader = new InputStreamReader(connection.getInputStream());

				if (urls[0].endsWith(".xml")) {
					SqliteElement.importXmlStreamToDb(
							dbOpener.getWritableDatabase(),
							reader,
							ReplaceStrategy.REPLACE_EXISTING);
				} else {
					ImportExport.INSTANCE.importDatabase(vncDatabase, reader);
				}
				return null;
			} catch (Exception e) {
				return e;
			}
		}

		@Override
		protected void onPostExecute(Exception e) {
			if(e == null) {
				finish();
				Log.d(TAG, "import successful!");
			} else {
				if(e instanceof MalformedURLException)
					errorNotify(getString(R.string.import_error_url) + " " + _textLoadUrl.getText(), e);

				if(e instanceof IOException)
					errorNotify(getString(R.string.import_error_io), e);

				if(e instanceof SAXException)
					errorNotify(getString(R.string.import_error_xml), e);
			}
		}
	}


	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.importexport);
		setTitle(R.string.import_export_settings);
		_textLoadUrl = findViewById(R.id.textImportUrl);
		_textSaveUrl = findViewById(R.id.textExportPath);

		dbOpener = new DbOpener(this);
		vncDatabase = VncDatabase.getInstance(this);

		File f;
		if(!Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
			Log.d(TAG, "external storage not mounted, using internal");
			f = getFilesDir();
		}
		else {
			Log.d(TAG, "external storage mounted, using it");
			f = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
		}

		f = new File(f, "MultiVNC-Export.json");

		_textSaveUrl.setText(f.getAbsolutePath());
		try {
			_textLoadUrl.setText(f.toURL().toString());
		} catch (MalformedURLException e) {
			// Do nothing; default value not set
		}

		mButtonExport = findViewById(R.id.buttonExport);
		mButtonExport.setOnClickListener(new View.OnClickListener() {

			@Override
			public void onClick(View v) {

				if(!checkPerm(R.id.buttonExport))
					return;

				try {
					Writer writer = new FileWriter(_textSaveUrl.getText().toString());

					ImportExport.INSTANCE.exportDatabase(vncDatabase, writer);

					writer.close();
					finish();
					Log.d(TAG, "export successful!");
				}
				catch (IOException ioe)
				{
					errorNotify("I/O Exception exporting config", ioe);
				}
			}

		});

		mButtonImport = findViewById(R.id.buttonImport);
		mButtonImport.setOnClickListener(new View.OnClickListener() {

			@Override
			public void onClick(View v) {

				if(!checkPerm(R.id.buttonImport))
					return;

				new ImportFromURLAsyncTask().execute(_textLoadUrl.getText().toString());
			}

		});
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		dbOpener.close();
	}

	@SuppressLint("NewApi")
	@Override
	public void onRequestPermissionsResult(int requestCode,  String[] permissions, int[] grantResults) {

		if(grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
			Log.d(TAG, "Permission result: given!");
			if(requestCode == R.id.buttonExport)
				mButtonExport.callOnClick();
			if(requestCode == R.id.buttonImport)
				mButtonImport.callOnClick();
		}
		else {
			Log.d(TAG, "Permission result: denied!");
			Utils.showErrorMessage(this, "Permission to access external storage was denied.");
		}

	}

	@SuppressWarnings("BooleanMethodIsAlwaysInverted") // holy shit
	private boolean checkPerm(int requestId) {

		if (android.os.Build.VERSION.SDK_INT >= 23) {

			if (checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
				Log.d(TAG, "Has no permission! Ask!");
				requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, requestId);
				return false;
			} else {
				Log.d(TAG, "Permission already given!");
			}
		}

		return true;
	}

	private void errorNotify(String msg, Throwable t)
	{
		Log.e(TAG, msg, t);
		Utils.showErrorMessage(this, msg + ": <br/><br/>" + t.getMessage());
	}

	/**
	 * Room does not provide us access to SQLiteOpenHelper which is required by
	 * XML importer. So we implement one here with essential features only.
	 */
	static class DbOpener extends SQLiteOpenHelper{

		DbOpener(Context context){
			super(context, VncDatabase.NAME,null, VncDatabase.VERSION);
		}

		@Override
		public void onCreate(SQLiteDatabase db) {
			//Nothing to do because creation is handled by VncDatabase
		}

		@Override
		public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
			//Nothing to do because migration is handled by VncDatabase
		}
	}
}
