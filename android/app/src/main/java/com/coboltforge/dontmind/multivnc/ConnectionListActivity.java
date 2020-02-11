/*
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 *
 * Copyright 2009,2010 Michael A. MacDonald
 */
package com.coboltforge.dontmind.multivnc;

import android.app.ListActivity;
import android.content.Intent;
import android.content.Intent.ShortcutIconResource;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;
import android.widget.ListView;
import android.widget.SimpleCursorAdapter;

/**
 * @author Michael A. MacDonald
 */
public class ConnectionListActivity extends ListActivity {

    VncDatabase database;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.connection_list);

        database = new VncDatabase(this);

        // Query for all people contacts using the Contacts.People convenience class.
        // Put a managed wrapper around the retrieved cursor so we don't have to worry about
        // requerying or closing it as the activity changes state.
        Cursor mCursor = database.getReadableDatabase().query(ConnectionBean.GEN_TABLE_NAME, new String[]{
                        ConnectionBean.GEN_FIELD__ID,
                        ConnectionBean.GEN_FIELD_NICKNAME,
                        ConnectionBean.GEN_FIELD_USERNAME,
                        ConnectionBean.GEN_FIELD_ADDRESS,
                        ConnectionBean.GEN_FIELD_PORT,
                        ConnectionBean.GEN_FIELD_REPEATERID},
                ConnectionBean.GEN_FIELD_KEEPPASSWORD + " <> 0", null, null, null, ConnectionBean.GEN_FIELD_NICKNAME);
        startManagingCursor(mCursor);

        // Now create a new list adapter bound to the cursor. 
        // SimpleListAdapter is designed for binding to a Cursor.
        SimpleCursorAdapter adapter = new SimpleCursorAdapter(
                this, // Context.
                R.layout.connection_list_item,
                mCursor,                                    // Pass in the cursor to bind to.
                new String[]{
                        ConnectionBean.GEN_FIELD_NICKNAME,
                        ConnectionBean.GEN_FIELD_ADDRESS,
                        ConnectionBean.GEN_FIELD_PORT,
                        ConnectionBean.GEN_FIELD_REPEATERID}, // Array of cursor columns to bind to.
                new int[]{
                        R.id.list_text_nickname,
                        R.id.list_text_address,
                        R.id.list_text_port,
                        R.id.list_text_repeater
                });                                 // Parallel array of which template objects to bind to those columns.

        // Bind to our new adapter.
        setListAdapter(adapter);
    }

    /* (non-Javadoc)
     * @see android.app.ListActivity#onListItemClick(android.widget.ListView, android.view.View, int, long)
     */
    @Override
    protected void onListItemClick(ListView l, View v, int position, long id) {
        ConnectionBean connection = new ConnectionBean();
        if (connection.Gen_read(database.getReadableDatabase(), id)) {
            // create shortcut if requested
            ShortcutIconResource icon = Intent.ShortcutIconResource.fromContext(this, R.drawable.ic_launcher);

            Intent intent = new Intent();

            Intent launchIntent = new Intent(this, VncCanvasActivity.class);
            Uri.Builder builder = new Uri.Builder();
            builder.authority(Constants.CONNECTION + ":" + connection.get_Id());
            builder.scheme("vnc");
            launchIntent.setData(builder.build());

            intent.putExtra(Intent.EXTRA_SHORTCUT_INTENT, launchIntent);
            intent.putExtra(Intent.EXTRA_SHORTCUT_NAME, connection.getNickname());
            intent.putExtra(Intent.EXTRA_SHORTCUT_ICON_RESOURCE, icon);

            setResult(RESULT_OK, intent);
        } else
            setResult(RESULT_CANCELED);

        finish();
    }

    /* (non-Javadoc)
     * @see android.app.Activity#onDestroy()
     */
    @Override
    protected void onDestroy() {
        database.close();
        super.onDestroy();
    }
}
