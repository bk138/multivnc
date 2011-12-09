package com.coboltforge.dontmind.multivnc;

import java.util.Hashtable;

/**
 * @author Christian Beier
 */

public interface ImDNSNotify {
	public void mDNSnotify(final String conn_name, final ConnectionBean conn, final Hashtable<String,ConnectionBean> connectionTable);
}
