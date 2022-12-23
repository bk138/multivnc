/**
 * Copyright (C) 2009 Michael A. MacDonald
 */
package com.coboltforge.dontmind.multivnc;

import android.content.Context;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.room.Database;
import androidx.room.Room;
import androidx.room.RoomDatabase;
import androidx.room.migration.Migration;
import androidx.sqlite.db.SupportSQLiteDatabase;

/**
 * Note: Version before migration to Room = 12
 */
@Database(entities = {ConnectionBean.class, MetaKeyBean.class, MetaList.class, SshKnownHost.class}, version = VncDatabase.VERSION, exportSchema = false)
public abstract class VncDatabase extends RoomDatabase {

    public static final int VERSION = 14;
    public static final String NAME = "VncDatabase";

    public abstract MetaListDao getMetaListDao();

    public abstract MetaKeyDao getMetaKeyDao();

    public abstract ConnectionDao getConnectionDao();

    public abstract SshKnownHostDao getSshKnownHostDao();


    private static VncDatabase instance = null;

    public static VncDatabase getInstance(Context context) {
        if (instance == null) {
            instance = Room.databaseBuilder(context, VncDatabase.class, NAME)
                    .allowMainThreadQueries()
                    .addMigrations(MIGRATION_12_13)
                    .addMigrations(MIGRATION_13_14)
                    .build();

            setupDefaultMetaList(instance);
        }
        return instance;
    }

    private static void setupDefaultMetaList(VncDatabase db) {
        if (db.getMetaListDao().getAll().isEmpty()) {
            MetaList l = new MetaList();
            l.id = 1;
            l.name = "DEFAULT";
            db.getMetaListDao().insert(l);
        }
    }


    //Room treats columns of primitive types as non-nullable but originally
    //these columns were not created with explicit nullability.
    //To solve this, ideally, we would simply alter the columns but SQLite
    //does not support that.
    //So we need to recreate these tables with correct nullability.
    private static final Migration MIGRATION_12_13 = new Migration(12, 13) {

        private String CREATE_NEW_CONNECTIONS_TABLE = "CREATE TABLE CONNECTION_BEAN_NEW (" +
                "_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL," +
                "NICKNAME TEXT," +
                "ADDRESS TEXT," +
                "PORT INTEGER NOT NULL," +
                "PASSWORD TEXT," +
                "COLORMODEL TEXT," +
                "FORCEFULL INTEGER NOT NULL," +
                "REPEATERID TEXT," +
                "INPUTMODE TEXT," +
                "SCALEMODE TEXT," +
                "USELOCALCURSOR INTEGER NOT NULL," +
                "KEEPPASSWORD INTEGER NOT NULL," +
                "FOLLOWMOUSE INTEGER NOT NULL," +
                "USEREPEATER INTEGER NOT NULL," +
                "METALISTID INTEGER NOT NULL," +
                "LAST_META_KEY_ID INTEGER NOT NULL," +
                "FOLLOWPAN INTEGER NOT NULL DEFAULT 0," +
                "USERNAME TEXT," +
                "SECURECONNECTIONTYPE TEXT," +
                "SHOWZOOMBUTTONS INTEGER  NOT NULL DEFAULT 1," +
                "DOUBLE_TAP_ACTION TEXT" +
                ")";

        private String CREATE_NEW_KEY_TABLE = "CREATE TABLE META_KEY_NEW (" +
                "_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL," +
                "METALISTID INTEGER NOT NULL," +
                "KEYDESC TEXT," +
                "METAFLAGS INTEGER NOT NULL," +
                "MOUSECLICK INTEGER NOT NULL," +
                "MOUSEBUTTONS INTEGER NOT NULL," +
                "KEYSYM INTEGER NOT NULL," +
                "SHORTCUT TEXT" +
                ")";

        private String CREATE_NEW_LIST_TABLE = "CREATE TABLE META_LIST_NEW (" +
                "_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL," +
                "NAME TEXT" +
                ")";

        @Override
        public void migrate(@NonNull SupportSQLiteDatabase database) {
            Log.i("VncDatabase", "Migrating to Room [12 -> 13]");

            //Create new
            database.execSQL(CREATE_NEW_CONNECTIONS_TABLE);
            database.execSQL(CREATE_NEW_KEY_TABLE);
            database.execSQL(CREATE_NEW_LIST_TABLE);

            //Copy over data
            database.execSQL("INSERT INTO CONNECTION_BEAN_NEW SELECT * FROM CONNECTION_BEAN");
            database.execSQL("INSERT INTO META_KEY_NEW SELECT * FROM META_KEY");
            database.execSQL("INSERT INTO META_LIST_NEW SELECT * FROM META_LIST");

            //Remove old
            database.execSQL("DROP TABLE CONNECTION_BEAN");
            database.execSQL("DROP TABLE META_KEY");
            database.execSQL("DROP TABLE META_LIST");

            //Rename new
            database.execSQL("ALTER TABLE CONNECTION_BEAN_NEW RENAME TO CONNECTION_BEAN");
            database.execSQL("ALTER TABLE META_KEY_NEW RENAME TO META_KEY");
            database.execSQL("ALTER TABLE META_LIST_NEW RENAME TO META_LIST");
        }
    };

    // this simply adds a SSH_KNOWN_HOST table
    private static final Migration MIGRATION_13_14 = new Migration(13, 14) {
        @Override
        public void migrate(@NonNull SupportSQLiteDatabase database) {
            Log.i("VncDatabase", "Migrating to Room [13 -> 14]");

            // add new columns to CONNECTION_BEAN
            database.execSQL("ALTER TABLE CONNECTION_BEAN ADD ENCODINGSSTRING TEXT NOT NULL DEFAULT 'tight zrle ultra copyrect hextile zlib corre rre trle zywrle raw'");
            database.execSQL("ALTER TABLE CONNECTION_BEAN ADD COMPRESSMODEL TEXT NOT NULL DEFAULT 'L0'");
            database.execSQL("ALTER TABLE CONNECTION_BEAN ADD QUALITYMODEL TEXT NOT NULL DEFAULT 'L5'");
            database.execSQL("ALTER TABLE CONNECTION_BEAN ADD SSH_HOST TEXT");
            database.execSQL("ALTER TABLE CONNECTION_BEAN ADD SSH_USERNAME TEXT");
            database.execSQL("ALTER TABLE CONNECTION_BEAN ADD SSH_PASSWORD TEXT");
            database.execSQL("ALTER TABLE CONNECTION_BEAN ADD SSH_PRIVKEY BLOB");
            database.execSQL("ALTER TABLE CONNECTION_BEAN ADD SSH_PRIVKEY_PASSWORD TEXT");

            //Create new
            database.execSQL("CREATE TABLE SSH_KNOWN_HOST (" +
                             "_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL," +
                             "HOST TEXT NOT NULL," +
                             "FINGERPRINT BLOB NOT NULL" +
                             ")");
        }
    };
}
