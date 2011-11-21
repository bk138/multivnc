package com.coboltforge.dontmind.multivnc;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;
import android.widget.ImageButton;
import android.widget.TextView;


public class AboutActivity extends Activity {

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.about);

		//Get version number/name and add it to the screen
		PackageInfo pinfo;
		try {
			pinfo = getPackageManager().getPackageInfo(getPackageName(),0);
			TextView tw = (TextView) findViewById(R.id.TextViewVersion);
			String v = getString(R.string.app_name) +  " " + pinfo.versionName;
			tw.setText(v);
		} catch (NameNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		ImageButton donateButton = (ImageButton) findViewById(R.id.paypal_button);
		donateButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				
				Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse("https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=HKRTWKNKBKPKN"));
		    	startActivity(browserIntent);
			}
		});
	}

}


