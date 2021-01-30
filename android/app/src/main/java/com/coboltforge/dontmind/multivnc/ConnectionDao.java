package com.coboltforge.dontmind.multivnc;

import android.database.Cursor;

import androidx.room.Dao;
import androidx.room.Delete;
import androidx.room.Insert;
import androidx.room.Query;
import androidx.room.Update;

import java.util.List;

@Dao
public interface ConnectionDao {

    @Query("SELECT * FROM CONNECTION_BEAN WHERE _id = :id")
    ConnectionBean get(long id);

    @Query("SELECT * FROM CONNECTION_BEAN")
    List<ConnectionBean> getAll();

    @Query("SELECT * FROM CONNECTION_BEAN WHERE KEEPPASSWORD <> 0 ORDER BY NICKNAME")
    Cursor getAllWithPassword();

    @Insert
    long insert(ConnectionBean c);

    @Update
    void update(ConnectionBean c);

    @Delete
    void delete(ConnectionBean c);

    /**
     * Update the given connection. If it is new then inserts it.
     */
    default void save(ConnectionBean c) {
        if (!c.keepPassword)
            c.password = "";

        if (c.id > 0)
            update(c);
        else
            c.id = insert(c);
    }
}
