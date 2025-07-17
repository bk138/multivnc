package com.coboltforge.dontmind.multivnc.ui;

import android.os.Build;
import android.os.Bundle;
import android.webkit.WebView;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;

import com.coboltforge.dontmind.multivnc.R;

public class HelpActivity extends AppCompatActivity {

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		// On Android 15 and later, calling enableEdgeToEdge ensures system bar icon colors update
		// when the device theme changes. Because calling it on pre-Android 15 has the side effect of
		// enabling EdgeToEdge there as well, we only use it on Android 15 and later.
		if (Build.VERSION.SDK_INT >= 35) {
			EdgeToEdge.enable(this);
		}
		setContentView(R.layout.help_activity);
		WebView wv = findViewById(R.id.helpwebView);
		wv.loadUrl("file:///android_asset/help.html");

	}
	
}
