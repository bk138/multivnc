/**
 * Copyright (C) 2009 Michael A. MacDonald
 */
package com.coboltforge.dontmind.multivnc;

import android.content.ContentValues;
import android.database.sqlite.SQLiteDatabase;
import android.widget.ImageView.ScaleType;

import com.antlersoft.android.dbimpl.NewInstance;

import java.lang.Comparable;

/**
 * @author Michael A. MacDonald
 *
 */
class ConnectionBean extends AbstractConnectionBean implements Comparable<ConnectionBean> {
	static final NewInstance<ConnectionBean> newInstance=new NewInstance<ConnectionBean>() {
		public ConnectionBean get() { return new ConnectionBean(); }
	};
	ConnectionBean()
	{
		set_Id(0);
		setAddress("");
		setPassword("");
		setKeepPassword(true);
		setNickname("");
		setUserName("");
		setPort(5900);
		setColorModel(COLORMODEL.C64.nameString());
		setScaleMode(ScaleType.MATRIX);
		setFollowMouse(true);
		setRepeaterId("");
		setMetaListId(1);
	}
	
	boolean isNew()
	{
		return get_Id()== 0;
	}
	
	void save(SQLiteDatabase database) {
		ContentValues values=Gen_getValues();
		values.remove(GEN_FIELD__ID);
		if ( ! getKeepPassword()) {
			values.put(GEN_FIELD_PASSWORD, "");
		}
		if ( isNew()) {
			set_Id(database.insert(GEN_TABLE_NAME, null, values));
		} else {
			database.update(GEN_TABLE_NAME, values, GEN_FIELD__ID + " = ?", new String[] { Long.toString(get_Id()) });
		}
	}
	
	ScaleType getScaleMode()
	{
		return ScaleType.valueOf(getScaleModeAsString());
	}
	
	void setScaleMode(ScaleType value)
	{
		setScaleModeAsString(value.toString());
	}
	
	@Override
	public String toString() {
		return get_Id() + " " + getNickname()+": "+getAddress()+", port "+getPort();
	}

	/* (non-Javadoc)
	 * @see java.lang.Comparable#compareTo(java.lang.Object)
	 */
	@Override
	public int compareTo(ConnectionBean another) {
		int result = getNickname().compareTo(another.getNickname());
		if (result == 0) {
			result = getAddress().compareTo(another.getAddress());
			if ( result == 0) {
				result = getPort() - another.getPort();
			}
		}
		return result;
	}
	
	/**
	 * parse host:port or [host]:port and split into address and port fields
	 * @param hostport_str
	 * @return true if there was a port, false if not
	 */
	boolean parseHostPort(String hostport_str) {
		int nr_colons = hostport_str.replaceAll("[^:]", "").length();
		int nr_endbrackets = hostport_str.replaceAll("[^]]", "").length();
		
		if (nr_colons == 1) { // IPv4
			String p = hostport_str.substring(hostport_str.indexOf(':') + 1);
			try {
				setPort(Integer.parseInt(p));
			} catch (Exception e) {
			}
			setAddress(hostport_str.substring(0, hostport_str.indexOf(':')));
			return true;
		}
		if(nr_colons > 1 && nr_endbrackets == 1) {
			String p = hostport_str.substring(hostport_str.indexOf(']') + 2); // it's [addr]:port
			try {
				setPort(Integer.parseInt(p));
			} catch (Exception e) {
			}
			setAddress(hostport_str.substring(0, hostport_str.indexOf(']') + 1));
			return true;
		}
		return false;
	}
}
