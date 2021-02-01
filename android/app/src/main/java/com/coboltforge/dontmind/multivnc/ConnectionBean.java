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
	private long id;

	@ColumnInfo(name = "NICKNAME")
	private String nickname;

	@ColumnInfo(name = "ADDRESS")
	private String address;

	@ColumnInfo(name = "PORT")
	private int port;

	@ColumnInfo(name = "PASSWORD")
	private String password;

	@ColumnInfo(name = "COLORMODEL")
	private String colorModel;

	@ColumnInfo(name = "FORCEFULL")
	private long forceFull;

	@ColumnInfo(name = "REPEATERID")
	private String repeaterId;

	@ColumnInfo(name = "INPUTMODE")
	private String inputMode;

	@ColumnInfo(name = "SCALEMODE")
	private String scalemode;

	@ColumnInfo(name = "USELOCALCURSOR")
	private boolean useLocalCursor;

	@ColumnInfo(name = "KEEPPASSWORD")
	private boolean keepPassword;

	@ColumnInfo(name = "FOLLOWMOUSE")
	private boolean followMouse;

	@ColumnInfo(name = "USEREPEATER")
	private boolean useRepeater;

	@ColumnInfo(name = "METALISTID")
	private long metaListId;

	@ColumnInfo(name = "LAST_META_KEY_ID")
	private long lastMetaKeyId;

	@ColumnInfo(name = "FOLLOWPAN", defaultValue = "0")
	private boolean followPan;

	@ColumnInfo(name = "USERNAME")
	private String userName;

	@ColumnInfo(name = "SECURECONNECTIONTYPE")
	private String secureConnectionType;

	@ColumnInfo(name = "SHOWZOOMBUTTONS", defaultValue = "1")
	private boolean showZoomButtons;

	@ColumnInfo(name = "DOUBLE_TAP_ACTION")
	private String doubleTapAction;

	ConnectionBean() {
		setId(0);
		setAddress("");
		setPassword("");
		setKeepPassword(true);
		setNickname("");
		setUserName("");
		setPort(5900);
		setColorModel(COLORMODEL.C24bit.nameString());
		setFollowMouse(true);
		setRepeaterId("");
		setMetaListId(1);
	}

	public long getId() {
		return id;
	}

	public void setId(long id) {
		this.id = id;
	}

	public String getNickname() {
		return nickname;
	}

	public void setNickname(String nickname) {
		this.nickname = nickname;
	}

	public String getAddress() {
		return address;
	}

	public void setAddress(String address) {
		this.address = address;
	}

	public int getPort() {
		return port;
	}

	public void setPort(int port) {
		this.port = port;
	}

	public String getPassword() {
		return password;
	}

	public void setPassword(String password) {
		this.password = password;
	}

	public String getColorModel() {
		return colorModel;
	}

	public void setColorModel(String colorModel) {
		this.colorModel = colorModel;
	}

	public long getForceFull() {
		return forceFull;
	}

	public void setForceFull(long forceFull) {
		this.forceFull = forceFull;
	}

	public String getRepeaterId() {
		return repeaterId;
	}

	public void setRepeaterId(String repeaterId) {
		this.repeaterId = repeaterId;
	}

	public String getInputMode() {
		return inputMode;
	}

	public void setInputMode(String inputMode) {
		this.inputMode = inputMode;
	}

	public String getScalemode() {
		return scalemode;
	}

	public void setScalemode(String scalemode) {
		this.scalemode = scalemode;
	}

	public boolean getUseLocalCursor() {
		return useLocalCursor;
	}

	public void setUseLocalCursor(boolean useLocalCursor) {
		this.useLocalCursor = useLocalCursor;
	}

	public boolean getKeepPassword() {
		return keepPassword;
	}

	public void setKeepPassword(boolean keepPassword) {
		this.keepPassword = keepPassword;
	}

	public boolean getFollowMouse() {
		return followMouse;
	}

	public void setFollowMouse(boolean followMouse) {
		this.followMouse = followMouse;
	}

	public boolean getUseRepeater() {
		return useRepeater;
	}

	public void setUseRepeater(boolean useRepeater) {
		this.useRepeater = useRepeater;
	}

	public long getMetaListId() {
		return metaListId;
	}

	public void setMetaListId(long metaListId) {
		this.metaListId = metaListId;
	}

	public long getLastMetaKeyId() {
		return lastMetaKeyId;
	}

	public void setLastMetaKeyId(long lastMetaKeyId) {
		this.lastMetaKeyId = lastMetaKeyId;
	}

	public boolean getFollowPan() {
		return followPan;
	}

	public void setFollowPan(boolean followPan) {
		this.followPan = followPan;
	}

	public String getUserName() {
		return userName;
	}

	public void setUserName(String userName) {
		this.userName = userName;
	}

	public String getSecureConnectionType() {
		return secureConnectionType;
	}

	public void setSecureConnectionType(String secureConnectionType) {
		this.secureConnectionType = secureConnectionType;
	}

	public boolean isShowZoomButtons() {
		return showZoomButtons;
	}

	public void setShowZoomButtons(boolean showZoomButtons) {
		this.showZoomButtons = showZoomButtons;
	}

	public String getDoubleTapAction() {
		return doubleTapAction;
	}

	public void setDoubleTapAction(String doubleTapAction) {
		this.doubleTapAction = doubleTapAction;
	}

	@Override
	public String toString() {
		return getId() + " " + getNickname() + ": " + getAddress() + ", port " + getPort();
	}

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
