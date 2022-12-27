package com.coboltforge.dontmind.multivnc;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.Enumeration;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.text.Html;
import android.util.Log;
import android.view.MotionEvent;

public class Utils {

	private static final String TAG = "Utils";

	public static void showErrorMessage(Context _context, String message) {
		showMessage(_context, _context.getString(R.string.utils_title_error), message, 0, android.R.attr.alertDialogIcon, null);
	}

	public static void showFatalErrorMessage(final Context _context, String message) {
		showMessage(_context, _context.getString(R.string.utils_title_error), message, 0, android.R.attr.alertDialogIcon, new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				((Activity) _context).finish();
			}
		});
	}

	public static void showMessage(Context _context, String title, String message, int icon, int iconAttribute, DialogInterface.OnClickListener ackHandler) {
		try {
			AlertDialog.Builder builder = new AlertDialog.Builder(_context);
			builder.setTitle(title);
			builder.setMessage(Html.fromHtml(message));
			builder.setCancelable(false);
			builder.setPositiveButton(android.R.string.ok, ackHandler);
			if(icon != 0)
				builder.setIcon(icon);
			if(iconAttribute != 0)
				builder.setIconAttribute(iconAttribute);
			builder.show();
		}
		catch(Exception e) {
		}
	}

	public static boolean DEBUG()
	{
		return BuildConfig.DEBUG;
	}

	public static void inspectEvent(MotionEvent e)
	{
		if(e == null)
			return;

		final int pointerCount = e.getPointerCount();

		Log.d(TAG, "Input: event time: " + e.getEventTime());
		for (int p = 0; p < pointerCount; p++) {
			Log.d(TAG, "Input:  pointer:" +
					e.getPointerId(p)
					+ " x:" + e.getX(p)
					+ " y:" + e.getY(p)
					+ " action:" + e.getAction());
		}
	}


	public static NetworkInterface getActiveNetworkInterface(Context c) {
		try {
			for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements();) {
				NetworkInterface intf = en.nextElement();

				for (Enumeration<InetAddress> enumIpAddr = intf.getInetAddresses(); enumIpAddr.hasMoreElements();) {
					InetAddress inetAddress = enumIpAddr.nextElement();
					if (inetAddress.isLoopbackAddress())
						break; // break inner loop, continue with outer loop

					return intf; // this is not the loopback and it has an IP address assigned
				}
			}
		} catch (SocketException e) {
			e.printStackTrace();
		}
		// nothing found
		return null;
	}


	public static InetAddress intToInetAddress(int hostAddress) {
		InetAddress inetAddress;
		byte[] addressBytes = { (byte) (0xff & hostAddress),
				(byte) (0xff & (hostAddress >> 8)),
				(byte) (0xff & (hostAddress >> 16)),
				(byte) (0xff & (hostAddress >> 24)) };

		try {
			inetAddress = InetAddress.getByAddress(addressBytes);
		} catch (UnknownHostException e) {
			return null;
		}
		return inetAddress;
	}

	public static void copy(InputStream in, OutputStream out) throws IOException {
		byte[] buffer = new byte[4096];
		int read;
		while ((read = in.read(buffer)) != -1) {
			out.write(buffer, 0, read);
		}
	}


}
