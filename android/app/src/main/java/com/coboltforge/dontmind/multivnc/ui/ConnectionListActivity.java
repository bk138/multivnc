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
package com.coboltforge.dontmind.multivnc.ui;

import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.widget.ListView;
import android.widget.SimpleCursorAdapter;

import androidx.activity.EdgeToEdge;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.pm.ShortcutInfoCompat;
import androidx.core.content.pm.ShortcutManagerCompat;
import androidx.core.graphics.drawable.IconCompat;
import androidx.fragment.app.ListFragment;

import com.coboltforge.dontmind.multivnc.Constants;
import com.coboltforge.dontmind.multivnc.R;
import com.coboltforge.dontmind.multivnc.db.ConnectionBean;
import com.coboltforge.dontmind.multivnc.db.VncDatabase;

/**
 * @author Michael A. MacDonald
 *
 */
public class ConnectionListActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // On Android 15 and later, calling enableEdgeToEdge ensures system bar icon colors update
        // when the device theme changes. Because calling it on pre-Android 15 has the side effect of
        // enabling EdgeToEdge there as well, we only use it on Android 15 and later.
        if (Build.VERSION.SDK_INT >= 35) {
            EdgeToEdge.enable(this);
        }

        setContentView(R.layout.connection_list_activity);

        if (savedInstanceState == null) {
            getSupportFragmentManager()
                    .beginTransaction()
                    .add(R.id.fragment_container, new ConnectionListFragment())
                    .commit();
        }
    }

    public static class ConnectionListFragment extends ListFragment {

        private VncDatabase database;

        @Override
        public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
            super.onViewCreated(view, savedInstanceState);
            setEmptyText(getString(R.string.shortcut_no_entries));

            database = VncDatabase.getInstance(getActivity());
            Cursor cursor = database.getConnectionDao().getAllAsCursor();
            getActivity().startManagingCursor(cursor);

            // Now create a new list adapter bound to the cursor.
            // SimpleListAdapter is designed for binding to a Cursor.
            SimpleCursorAdapter adapter = new SimpleCursorAdapter(
                    getActivity(), // Context
                    R.layout.connection_list_item,
                    cursor,        // Pass in the cursor to bind to.
                    new String[] {
                            ConnectionBean.GEN_FIELD_NICKNAME,
                            ConnectionBean.GEN_FIELD_ADDRESS,
                            ConnectionBean.GEN_FIELD_PORT,
                            ConnectionBean.GEN_FIELD_REPEATERID }, // Array of cursor columns to bind to.
                    new int[] {
                            R.id.list_text_nickname,
                            R.id.list_text_address,
                            R.id.list_text_port,
                            R.id.list_text_repeater
                    });  // Parallel array of which template objects to bind to those columns.
            // Bind to our new adapter.
            setListAdapter(adapter);
        }

        @Override
        public void onListItemClick(@NonNull ListView l, @NonNull View v, int position, long id) {
            ConnectionBean connection = database.getConnectionDao().get(id);
            if (connection != null) {
                // Intent that is used when tapping shortcut
                Intent launchIntent = new Intent(getActivity(), VncCanvasActivity.class);
                Uri.Builder builder = new Uri.Builder();
                builder.authority(Constants.CONNECTION + ":" + connection.id);
                builder.scheme("vnc");
                launchIntent.setData(builder.build());
                launchIntent.setAction(Intent.ACTION_VIEW); // important on Android 14+

                ShortcutInfoCompat shortcut = new ShortcutInfoCompat.Builder(getActivity(), getActivity().getPackageName() + "-" + connection.id)
                        .setShortLabel(connection.nickname)
                        .setIcon(IconCompat.createWithResource(getActivity(), R.drawable.ic_launcher))
                        .setIntent(launchIntent)
                        .build();

                getActivity().setResult(AppCompatActivity.RESULT_OK, ShortcutManagerCompat.createShortcutResultIntent(getActivity(), shortcut));
            } else {
                getActivity().setResult(AppCompatActivity.RESULT_CANCELED);
            }

            getActivity().finish();
        }
    }
}
