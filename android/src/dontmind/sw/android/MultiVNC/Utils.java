package dontmind.sw.android.MultiVNC;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.AlertDialog;
import android.app.ActivityManager.MemoryInfo;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.text.Html;

public class Utils {
	
	public static boolean debug = false;

	public static void showYesNoPrompt(Context _context, String title, String message, OnClickListener onYesListener, OnClickListener onNoListener) {
		AlertDialog.Builder builder = new AlertDialog.Builder(_context);
		builder.setTitle(title);
		builder.setIcon(android.R.drawable.ic_dialog_info); // lame icon
		builder.setMessage(message);
		builder.setCancelable(false);
		builder.setPositiveButton("Yes", onYesListener);
		builder.setNegativeButton("No", onNoListener);
		builder.show();
	}
	
	public static ActivityManager getActivityManager(Context context)
	{
		ActivityManager result = (ActivityManager)context.getSystemService(Context.ACTIVITY_SERVICE);
		if (result == null)
			throw new UnsupportedOperationException("Could not retrieve ActivityManager");
		return result;
	}
	
	public static MemoryInfo getMemoryInfo(Context _context) {
		MemoryInfo info = new MemoryInfo();
		getActivityManager(_context).getMemoryInfo(info);
		return info;
	}
	
	private static int nextNoticeID = 0;
	public static int nextNoticeID() {
		nextNoticeID++;
		return nextNoticeID;
	}

	public static void showErrorMessage(Context _context, String message) {
		showMessage(_context, "Error!", message, android.R.drawable.ic_dialog_alert, null);
	}

	public static void showFatalErrorMessage(final Context _context, String message) {
		showMessage(_context, "Error!", message, android.R.drawable.ic_dialog_alert, new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				((Activity) _context).finish();
			}
		});
	}
	
	public static void showMessage(Context _context, String title, String message, int icon, DialogInterface.OnClickListener ackHandler) {
		AlertDialog.Builder builder = new AlertDialog.Builder(_context);
		builder.setTitle(title);
		builder.setMessage(Html.fromHtml(message));
		builder.setCancelable(false);
		builder.setPositiveButton("Acknowledged", ackHandler);
		builder.setIcon(icon);
		builder.show();
	}
	
	public static void DEBUG(Context c)
	{
		try {
			debug = (c.getPackageManager().getApplicationInfo(c.getPackageName(), 0).flags &
					ApplicationInfo.FLAG_DEBUGGABLE) != 0;
		} catch (NameNotFoundException e) {
			e.printStackTrace();
		}
	}
	
	public static boolean DEBUG()
	{
		return debug;
	}
}
