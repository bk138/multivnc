/**
 * Copyright (C) 2009 Michael A. MacDonald
 */
package com.coboltforge.dontmind.multivnc;

import android.content.ContentValues;

import androidx.room.ColumnInfo;
import androidx.room.Entity;
import androidx.room.PrimaryKey;

/**
 * @author Michael A. MacDonald
 */
@Entity(tableName = "CONNECTION_BEAN")
class ConnectionBean implements Comparable<ConnectionBean> {

	@PrimaryKey(autoGenerate = true)
	@ColumnInfo(name = "_id")
	public long id;

	@ColumnInfo(name = "NICKNAME")
	public String nickname;

	@ColumnInfo(name = "ADDRESS")
	public String address;

	@ColumnInfo(name = "PORT")
	public int port;

	@ColumnInfo(name = "PASSWORD")
	public String password;

	@ColumnInfo(name = "COLORMODEL")
	public String colorModel;

	@ColumnInfo(name = "FORCEFULL")
	public long forceFull;

	@ColumnInfo(name = "REPEATERID")
	public String repeaterId;

	@ColumnInfo(name = "INPUTMODE")
	public String inputMode;

	@ColumnInfo(name = "SCALEMODE")
	public String scalemode;

	@ColumnInfo(name = "USELOCALCURSOR")
	public boolean useLocalCursor;

	@ColumnInfo(name = "KEEPPASSWORD")
	public boolean keepPassword;

	@ColumnInfo(name = "FOLLOWMOUSE")
	public boolean followMouse;

	@ColumnInfo(name = "USEREPEATER")
	public boolean useRepeater;

	@ColumnInfo(name = "METALISTID")
	public long metaListId;

	@ColumnInfo(name = "LAST_META_KEY_ID")
	public long lastMetaKeyId;

	@ColumnInfo(name = "FOLLOWPAN", defaultValue = "0")
	public boolean followPan;

	@ColumnInfo(name = "USERNAME")
	public String userName;

	@ColumnInfo(name = "SECURECONNECTIONTYPE")
	public String secureConnectionType;

	@ColumnInfo(name = "SHOWZOOMBUTTONS", defaultValue = "1")
	public boolean showZoomButtons;

	@ColumnInfo(name = "DOUBLE_TAP_ACTION")
	public String doubleTapAction;

	ConnectionBean() {
		id = 0;
		address = "";
		password = "";
		keepPassword = true;
		nickname = "";
		userName = "";
		port = 5900;
		colorModel = COLORMODEL.C24bit.nameString();
		followMouse = true;
		repeaterId = "";
		metaListId = 1;
	}

	@Override
	public String toString() {
		return id + " " + nickname + ": " + address + ", port " + port;
	}

	@Override
	public int compareTo(ConnectionBean another) {
		int result = nickname.compareTo(another.nickname);
		if (result == 0) {
			result = address.compareTo(another.address);
			if (result == 0) {
				result = port - another.port;
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
				port = Integer.parseInt(p);
			} catch (Exception e) {
			}
			address = hostport_str.substring(0, hostport_str.indexOf(':'));
			return true;
		}
		if(nr_colons > 1 && nr_endbrackets == 1) {
			String p = hostport_str.substring(hostport_str.indexOf(']') + 2); // it's [addr]:port
			try {
				port = Integer.parseInt(p);
			} catch (Exception e) {
			}
			address = hostport_str.substring(0, hostport_str.indexOf(']') + 1);
			return true;
		}
		return false;
	}

	/**********************************************************************************************
	 *  Serialization helpers
	 *  These are used to parcel ConnectionBean through Intents.
	 *********************************************************************************************/

	public static final String GEN_FIELD__ID = "_id";
	public static final String GEN_FIELD_NICKNAME = "NICKNAME";
	public static final String GEN_FIELD_ADDRESS = "ADDRESS";
	public static final String GEN_FIELD_PORT = "PORT";
	public static final String GEN_FIELD_PASSWORD = "PASSWORD";
	public static final String GEN_FIELD_COLORMODEL = "COLORMODEL";
	public static final String GEN_FIELD_FORCEFULL = "FORCEFULL";
	public static final String GEN_FIELD_REPEATERID = "REPEATERID";
	public static final String GEN_FIELD_INPUTMODE = "INPUTMODE";
	public static final String GEN_FIELD_SCALEMODE = "SCALEMODE";
	public static final String GEN_FIELD_USELOCALCURSOR = "USELOCALCURSOR";
	public static final String GEN_FIELD_KEEPPASSWORD = "KEEPPASSWORD";
	public static final String GEN_FIELD_FOLLOWMOUSE = "FOLLOWMOUSE";
	public static final String GEN_FIELD_USEREPEATER = "USEREPEATER";
	public static final String GEN_FIELD_METALISTID = "METALISTID";
	public static final String GEN_FIELD_LAST_META_KEY_ID = "LAST_META_KEY_ID";
	public static final String GEN_FIELD_FOLLOWPAN = "FOLLOWPAN";
	public static final String GEN_FIELD_USERNAME = "USERNAME";
	public static final String GEN_FIELD_SECURECONNECTIONTYPE = "SECURECONNECTIONTYPE";
	public static final String GEN_FIELD_SHOWZOOMBUTTONS = "SHOWZOOMBUTTONS";
	public static final String GEN_FIELD_DOUBLE_TAP_ACTION = "DOUBLE_TAP_ACTION";

	public ContentValues Gen_getValues() {
		ContentValues values = new ContentValues();
		values.put(GEN_FIELD__ID, Long.toString(id));
		values.put(GEN_FIELD_NICKNAME, nickname);
		values.put(GEN_FIELD_ADDRESS, address);
		values.put(GEN_FIELD_PORT, Integer.toString(port));
		values.put(GEN_FIELD_PASSWORD, password);
		values.put(GEN_FIELD_COLORMODEL, colorModel);
		values.put(GEN_FIELD_FORCEFULL, Long.toString(forceFull));
		values.put(GEN_FIELD_REPEATERID, repeaterId);
		values.put(GEN_FIELD_INPUTMODE, inputMode);
		values.put(GEN_FIELD_SCALEMODE, scalemode);
		values.put(GEN_FIELD_USELOCALCURSOR, (useLocalCursor ? "1" : "0"));
		values.put(GEN_FIELD_KEEPPASSWORD, (keepPassword ? "1" : "0"));
		values.put(GEN_FIELD_FOLLOWMOUSE, (followMouse ? "1" : "0"));
		values.put(GEN_FIELD_USEREPEATER, (useRepeater ? "1" : "0"));
		values.put(GEN_FIELD_METALISTID, Long.toString(metaListId));
		values.put(GEN_FIELD_LAST_META_KEY_ID, Long.toString(lastMetaKeyId));
		values.put(GEN_FIELD_FOLLOWPAN, (followPan ? "1" : "0"));
		values.put(GEN_FIELD_USERNAME, userName);
		values.put(GEN_FIELD_SECURECONNECTIONTYPE, secureConnectionType);
		values.put(GEN_FIELD_SHOWZOOMBUTTONS, (showZoomButtons ? "1" : "0"));
		values.put(GEN_FIELD_DOUBLE_TAP_ACTION, doubleTapAction);
		return values;
	}

	/**
	 * Populate one instance from a ContentValues
	 */
	public void Gen_populate(ContentValues values) {
		id = values.getAsLong(GEN_FIELD__ID);
		nickname = values.getAsString(GEN_FIELD_NICKNAME);
		address = values.getAsString(GEN_FIELD_ADDRESS);
		port = values.getAsInteger(GEN_FIELD_PORT);
		password = values.getAsString(GEN_FIELD_PASSWORD);
		colorModel = values.getAsString(GEN_FIELD_COLORMODEL);
		forceFull = values.getAsLong(GEN_FIELD_FORCEFULL);
		repeaterId = values.getAsString(GEN_FIELD_REPEATERID);
		inputMode = values.getAsString(GEN_FIELD_INPUTMODE);
		scalemode = values.getAsString(GEN_FIELD_SCALEMODE);
		useLocalCursor = (values.getAsInteger(GEN_FIELD_USELOCALCURSOR) != 0);
		keepPassword = (values.getAsInteger(GEN_FIELD_KEEPPASSWORD) != 0);
		followMouse = (values.getAsInteger(GEN_FIELD_FOLLOWMOUSE) != 0);
		useRepeater = (values.getAsInteger(GEN_FIELD_USEREPEATER) != 0);
		metaListId = values.getAsLong(GEN_FIELD_METALISTID);
		lastMetaKeyId = values.getAsLong(GEN_FIELD_LAST_META_KEY_ID);
		followPan = (values.getAsInteger(GEN_FIELD_FOLLOWPAN) != 0);
		userName = values.getAsString(GEN_FIELD_USERNAME);
		secureConnectionType = values.getAsString(GEN_FIELD_SECURECONNECTIONTYPE);
		showZoomButtons = (values.getAsInteger(GEN_FIELD_SHOWZOOMBUTTONS) != 0);
		doubleTapAction = values.getAsString(GEN_FIELD_DOUBLE_TAP_ACTION);
	}
}
