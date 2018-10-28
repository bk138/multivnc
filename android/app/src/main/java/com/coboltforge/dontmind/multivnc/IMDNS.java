package com.coboltforge.dontmind.multivnc;

import java.util.Hashtable;

/**
 * @author Christian Beier
 */

public interface IMDNS {
	public void onMDNSstartupCompleted(boolean wasSuccessful);
	public void onMDNSnotify(final String conn_name, final ConnectionBean conn, final Hashtable<String,ConnectionBean> connectionTable);
}
