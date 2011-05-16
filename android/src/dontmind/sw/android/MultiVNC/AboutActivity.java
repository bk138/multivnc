package dontmind.sw.android.MultiVNC;

import android.app.Activity;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Bundle;
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
			String v = getString(R.string.app_name) +  " " + pinfo.versionName + " ("+pinfo.versionCode + ")";
			tw.setText(v);
		} catch (NameNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

}


