package com.coboltforge.dontmind.multivnc;

import androidx.room.Dao;
import androidx.room.Insert;
import androidx.room.Query;
import androidx.room.Update;

import java.util.List;

@Dao
interface MetaListDao {

    @Query("SELECT * FROM META_LIST WHERE _id = :id")
    MetaList get(long id);

    @Insert
    long insert(MetaList list);

    @Update
    void update(MetaList list);

    @Query("SELECT * FROM META_LIST")
    List<MetaList> getAll();
}
