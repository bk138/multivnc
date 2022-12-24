/**
 * Copyright (C) 2009 Michael A. MacDonald
 */
package com.coboltforge.dontmind.multivnc;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.AdapterView;
import android.widget.CompoundButton;
import android.widget.Spinner;
import android.widget.TextView;

import com.coboltforge.dontmind.multivnc.db.ConnectionBean;
import com.coboltforge.dontmind.multivnc.db.MetaKeyBase;
import com.coboltforge.dontmind.multivnc.db.MetaKeyBean;
import com.coboltforge.dontmind.multivnc.db.MetaList;
import com.coboltforge.dontmind.multivnc.db.VncDatabase;

/**
 * @author Michael A. MacDonald
 *
 */
class MetaKeyDialog extends Dialog {

	private final static String TAG = "MetaKeyDialog";

	CheckBox _checkShift;
	CheckBox _checkCtrl;
	CheckBox _checkAlt;
	CheckBox _checkSuper;
	TextView _textKeyDesc;
	Spinner _spinnerKeySelect;

	VncDatabase _database;
	static List<MetaList> _lists;
	ArrayList<MetaKeyBean> _keysInList = new ArrayList<>();
	long _listId;
	VncCanvasActivity _canvasActivity;
	MetaKeyBean _currentKeyBean;

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
		setContentView(R.layout.metakey_dialog);
		setTitle(R.string.meta_key_title);
		_checkShift = (CheckBox)findViewById(R.id.checkboxShift);
		_checkCtrl = (CheckBox)findViewById(R.id.checkboxCtrl);
		_checkAlt = (CheckBox)findViewById(R.id.checkboxAlt);
		_checkSuper = (CheckBox)findViewById(R.id.checkboxSuper);
		_textKeyDesc = (TextView)findViewById(R.id.textKeyDesc);
		_spinnerKeySelect = (Spinner)findViewById(R.id.spinnerKeySelect);

		_database = _canvasActivity.database;
		if (_lists == null) {
			_lists = _database.getMetaListDao().getAll();
		}
		_spinnerKeySelect.setAdapter(new ArrayAdapter<String>(getOwnerActivity(), android.R.layout.simple_spinner_item, MetaKeyBean.allKeysNames));
		_spinnerKeySelect.setSelection(0);

		_checkShift.setOnCheckedChangeListener(new MetaCheckListener(VNCConn.SHIFT_MASK));
		_checkAlt.setOnCheckedChangeListener(new MetaCheckListener(VNCConn.ALT_MASK));
		_checkSuper.setOnCheckedChangeListener(new MetaCheckListener(VNCConn.SUPER_MASK));
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
			int currentFlags = _currentKeyBean.metaFlags;
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
		if (index < 0)
		{
			int insertionPoint = -(index + 1);
			_currentKeyBean.id = _database.getMetaKeyDao().insert(_currentKeyBean);
			_keysInList.add(insertionPoint,_currentKeyBean);
			_connection.lastMetaKeyId = _currentKeyBean.id;
		}
		else
		{
			MetaKeyBean bean = _keysInList.get(index);
			_connection.lastMetaKeyId = bean.id;
		}
		_database.getConnectionDao().update(_connection);
		_canvasActivity.lastSentKey = _currentKeyBean;
		_canvasActivity.vncCanvas.sendMetaKey(_currentKeyBean);
		Log.d(TAG, "sendCurrentKey: sent " + _currentKeyBean.getKeyDesc());
	}

	void setMetaKeyList()
	{
		long listId = _connection.metaListId;
		if (listId!=_listId) {
			for (int i=0; i<_lists.size(); ++i)
			{
				MetaList list = _lists.get(i);
				if (list.id==listId)
				{
					_keysInList.clear();
					_keysInList.addAll(_database.getMetaKeyDao().getByMetaList(listId));

					ArrayList<String> keys = new ArrayList<String>(_keysInList.size());
					int selectedOffset = 0;
					long lastSelectedKeyId = _canvasActivity.getConnection().lastMetaKeyId;
					for (int j=0; j<_keysInList.size(); j++)
					{
						MetaKeyBean key = _keysInList.get(j);
						keys.add( key.getKeyDesc());
						if (lastSelectedKeyId==key.id)
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
		int flags = _currentKeyBean.metaFlags;
		_checkAlt.setChecked(0 != (flags & VNCConn.ALT_MASK));
		_checkShift.setChecked(0 != (flags & VNCConn.SHIFT_MASK));
		_checkCtrl.setChecked(0 != (flags & VNCConn.CTRL_MASK));
		_checkSuper.setChecked(0 != (flags & VNCConn.SUPER_MASK));
		MetaKeyBase base = null;
		if (_currentKeyBean.isMouseClick)
		{
			base = MetaKeyBean.keysByMouseButton.get(_currentKeyBean.mouseButtons);
		} else {
			base = MetaKeyBean.keysByKeySym.get(_currentKeyBean.keySym);
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
				_currentKeyBean.setMetaFlags(_currentKeyBean.metaFlags | _mask);
			}
			else
			{
				_currentKeyBean.setMetaFlags(_currentKeyBean.metaFlags & ~_mask);
			}
			_textKeyDesc.setText(_currentKeyBean.getKeyDesc());
		}
	}

}
