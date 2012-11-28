/**
 * Copyright (C) 2009 Michael A. MacDonald
 */
package com.coboltforge.dontmind.multivnc;

import java.text.MessageFormat;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Map.Entry;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.AdapterView;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TextView;

/**
 * @author Michael A. MacDonald
 *
 */
class MetaKeyDialog extends Dialog implements ConnectionSettable {

	CheckBox _checkShift;
	CheckBox _checkCtrl;
	CheckBox _checkAlt;
	TextView _textKeyDesc;
	Spinner _spinnerKeySelect;

	VncDatabase _database;
	static ArrayList<MetaList> _lists;
	ArrayList<MetaKeyBean> _keysInList;
	long _listId;
	VncCanvasActivity _canvasActivity;
	MetaKeyBean _currentKeyBean;

	static final String[] EMPTY_ARGS = new String[0];

	ConnectionBean _connection;

	/**
	 * @param context
	 */
	public MetaKeyDialog(Context context) {
		super(context);
		setOwnerActivity((Activity)context);
		_canvasActivity = (VncCanvasActivity)context;
	}


	/* (non-Javadoc)
	 * @see android.app.Dialog#onCreate(android.os.Bundle)
	 */
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.metakey);
		setTitle(R.string.meta_key_title);
		_checkShift = (CheckBox)findViewById(R.id.checkboxShift);
		_checkCtrl = (CheckBox)findViewById(R.id.checkboxCtrl);
		_checkAlt = (CheckBox)findViewById(R.id.checkboxAlt);
		_textKeyDesc = (TextView)findViewById(R.id.textKeyDesc);
		_spinnerKeySelect = (Spinner)findViewById(R.id.spinnerKeySelect);

		_database = _canvasActivity.database;
		if (_lists == null) {
			_lists = new ArrayList<MetaList>();
			MetaList.getAll(_database.getReadableDatabase(), MetaList.GEN_TABLE_NAME, _lists, MetaList.GEN_NEW);
		}
		_spinnerKeySelect.setAdapter(new ArrayAdapter<String>(getOwnerActivity(), android.R.layout.simple_spinner_item, MetaKeyBean.allKeysNames));
		_spinnerKeySelect.setSelection(0);

		setListSpinner();

		_checkShift.setOnCheckedChangeListener(new MetaCheckListener(VNCConn.SHIFT_MASK));
		_checkAlt.setOnCheckedChangeListener(new MetaCheckListener(VNCConn.ALT_MASK));
		_checkCtrl.setOnCheckedChangeListener(new MetaCheckListener(VNCConn.CTRL_MASK));




		_spinnerKeySelect.setOnItemSelectedListener(new OnItemSelectedListener() {

			/* (non-Javadoc)
			 * @see android.widget.AdapterView.OnItemSelectedListener#onItemSelected(android.widget.AdapterView, android.view.View, int, long)
			 */
			public void onItemSelected(AdapterView<?> parent, View view,
					int position, long id) {
				if (_currentKeyBean == null) {
					_currentKeyBean = new MetaKeyBean(0,0,MetaKeyBean.allKeys.get(position));
				}
				else {
					_currentKeyBean.setKeyBase(MetaKeyBean.allKeys.get(position));
				}
				updateDialogForCurrentKey();
			}

			/* (non-Javadoc)
			 * @see android.widget.AdapterView.OnItemSelectedListener#onNothingSelected(android.widget.AdapterView)
			 */
			public void onNothingSelected(AdapterView<?> parent) {
			}
		});

		((Button)findViewById(R.id.buttonSend)).setOnClickListener(new View.OnClickListener() {

			/* (non-Javadoc)
			 * @see android.view.View.OnClickListener#onClick(android.view.View)
			 */
			public void onClick(View v) {
				sendCurrentKey();
				try{
					dismiss();
				}
				catch(Exception e) {
				}
			}

		});



	}

	private boolean _justStarted;

	/* (non-Javadoc)
	 * @see android.app.Dialog#onStart()
	 */
	@Override
	protected void onStart() {
		takeKeyEvents(true);
		_justStarted = true;
		super.onStart();
		View v = getCurrentFocus();
		if (v!=null)
			v.clearFocus();
	}

	/* (non-Javadoc)
	 * @see android.app.Dialog#onStop()
	 */
	@Override
	protected void onStop() {
		takeKeyEvents(false);
		super.onStop();
	}

	/* (non-Javadoc)
	 * @see android.app.Dialog#onKeyDown(int, android.view.KeyEvent)
	 */
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		_justStarted = false;
		if (keyCode != KeyEvent.KEYCODE_BACK && keyCode != KeyEvent.KEYCODE_MENU && getCurrentFocus() == null)
		{
			int flags = event.getMetaState();
			int currentFlags = _currentKeyBean.getMetaFlags();
			MetaKeyBase base = MetaKeyBean.keysByKeyCode.get(keyCode);
			if (base != null)
			{
				if (0 != (flags & KeyEvent.META_SHIFT_ON))
				{
					currentFlags |= VNCConn.SHIFT_MASK;
				}
				if (0 != (flags & KeyEvent.META_ALT_ON))
				{
					currentFlags |= VNCConn.ALT_MASK;
				}
				_currentKeyBean.setKeyBase(base);
			}
			else
			{
				// Toggle flags according to meta keys
				if (0 != (flags & KeyEvent.META_SHIFT_ON))
				{
					currentFlags ^= VNCConn.SHIFT_MASK;
				}
				if (0 != (flags & KeyEvent.META_ALT_ON))
				{
					currentFlags ^= VNCConn.ALT_MASK;
				}
				if (keyCode == KeyEvent.KEYCODE_SEARCH)
				{
					currentFlags ^= VNCConn.CTRL_MASK;
				}
			}
			_currentKeyBean.setMetaFlags(currentFlags);
			updateDialogForCurrentKey();
			return true;
		}
		return super.onKeyDown(keyCode, event);
	}

	/* (non-Javadoc)
	 * @see android.app.Dialog#onKeyUp(int, android.view.KeyEvent)
	 */
	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event) {
		if (! _justStarted && keyCode != KeyEvent.KEYCODE_BACK && keyCode != KeyEvent.KEYCODE_MENU && getCurrentFocus()==null)
		{
			if (MetaKeyBean.keysByKeyCode.get(keyCode) != null)
			{
				sendCurrentKey();
				try{
					dismiss();
				}
				catch(Exception e) {
				}
			}
			return true;
		}
		_justStarted = false;
		return super.onKeyUp(keyCode, event);
	}


	void sendCurrentKey()
	{
		int index = Collections.binarySearch(_keysInList, _currentKeyBean);
		SQLiteDatabase db = _database.getWritableDatabase();
		if (index < 0)
		{
			int insertionPoint = -(index + 1);
			_currentKeyBean.Gen_insert(db);
			_keysInList.add(insertionPoint,_currentKeyBean);
			_connection.setLastMetaKeyId(_currentKeyBean.get_Id());
		}
		else
		{
			MetaKeyBean bean = _keysInList.get(index);
			_connection.setLastMetaKeyId(bean.get_Id());
		}
		_connection.Gen_update(db);
		_canvasActivity.vncCanvas.sendMetaKey(_currentKeyBean);
	}

	void setMetaKeyList()
	{
		long listId = _connection.getMetaListId();
		if (listId!=_listId) {
			for (int i=0; i<_lists.size(); ++i)
			{
				MetaList list = _lists.get(i);
				if (list.get_Id()==listId)
				{
					_keysInList = new ArrayList<MetaKeyBean>();
					Cursor c = _database.getReadableDatabase().rawQuery(
							MessageFormat.format("SELECT * FROM {0} WHERE {1} = {2} ORDER BY KEYDESC",
									MetaKeyBean.GEN_TABLE_NAME,
									MetaKeyBean.GEN_FIELD_METALISTID,
									listId),
							EMPTY_ARGS);
					MetaKeyBean.Gen_populateFromCursor(
							c,
							_keysInList,
							MetaKeyBean.NEW);
					c.close();
					ArrayList<String> keys = new ArrayList<String>(_keysInList.size());
					int selectedOffset = 0;
					long lastSelectedKeyId = _canvasActivity.getConnection().getLastMetaKeyId();
					for (int j=0; j<_keysInList.size(); j++)
					{
						MetaKeyBean key = _keysInList.get(j);
						keys.add( key.getKeyDesc());
						if (lastSelectedKeyId==key.get_Id())
						{
							selectedOffset = j;
						}
					}
					if (keys.size()>0)
					{
						_currentKeyBean = new MetaKeyBean(_keysInList.get(selectedOffset));
					}
					else
					{
						_currentKeyBean = new MetaKeyBean(listId, 0, MetaKeyBean.allKeys.get(0));
					}
					updateDialogForCurrentKey();
					break;
				}
			}
			_listId = listId;
		}
	}

	private void updateDialogForCurrentKey()
	{
		int flags = _currentKeyBean.getMetaFlags();
		_checkAlt.setChecked(0 != (flags & VNCConn.ALT_MASK));
		_checkShift.setChecked(0 != (flags & VNCConn.SHIFT_MASK));
		_checkCtrl.setChecked(0 != (flags & VNCConn.CTRL_MASK));
		MetaKeyBase base = null;
		if (_currentKeyBean.isMouseClick())
		{
			base = MetaKeyBean.keysByMouseButton.get(_currentKeyBean.getMouseButtons());
		} else {
			base = MetaKeyBean.keysByKeySym.get(_currentKeyBean.getKeySym());
		}
		if (base != null) {
			int index = Collections.binarySearch(MetaKeyBean.allKeys,base);
			if (index >= 0) {
				_spinnerKeySelect.setSelection(index);
			}
		}
		_textKeyDesc.setText(_currentKeyBean.getKeyDesc());
	}

	public void setConnection(ConnectionBean conn)
	{
		if ( _connection != conn) {
			_connection = conn;
			setMetaKeyList();
		}
	}

	void setListSpinner()
	{
		ArrayList<String> listNames = new ArrayList<String>(_lists.size());
		for (int i=0; i<_lists.size(); ++i)
		{
			MetaList l = _lists.get(i);
			listNames.add(l.getName());
		}
	}

	/**
	 * @author Michael A. MacDonald
	 *
	 */
	class MetaCheckListener implements OnCheckedChangeListener {

		private int _mask;

		MetaCheckListener(int mask) {
			_mask = mask;
		}
		/* (non-Javadoc)
		 * @see android.widget.CompoundButton.OnCheckedChangeListener#onCheckedChanged(android.widget.CompoundButton, boolean)
		 */
		@Override
		public void onCheckedChanged(CompoundButton buttonView,
				boolean isChecked) {
			if (isChecked)
			{
				_currentKeyBean.setMetaFlags(_currentKeyBean.getMetaFlags() | _mask);
			}
			else
			{
				_currentKeyBean.setMetaFlags(_currentKeyBean.getMetaFlags() & ~_mask);
			}
			_textKeyDesc.setText(_currentKeyBean.getKeyDesc());
		}
	}

}
