package com.coboltforge.dontmind.multivnc;


/*
 * About activity for MultiVNC.
 * 
 * Copyright © 2011-2012 Christian Beier <dontmind@freeshell.org>
 */


import com.google.ads.*;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.net.Uri;
import android.os.Bundle;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
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
			e.printStackTrace();
		}
		
		Button loveButton = (Button) findViewById(R.id.love_button);
		loveButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View arg0) {
				startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse("market://details?id=" + getPackageName())));
			}
		});
		
		ImageButton donateButton = (ImageButton) findViewById(R.id.paypal_button);
		donateButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				
				Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse("https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=HKRTWKNKBKPKN"));
		    	startActivity(browserIntent);
			}
		});
		
		// Look up the AdView as a resource and load a request.
	    AdView adView = (AdView)this.findViewById(R.id.ad);
	    adView.loadAd(new AdRequest());
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

}


