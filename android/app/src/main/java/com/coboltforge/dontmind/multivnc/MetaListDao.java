package com.coboltforge.dontmind.multivnc;

import androidx.room.Dao;
import androidx.room.Insert;
import androidx.room.Query;

import java.util.List;

@Dao
interface MetaListDao {

    @Insert
    long insert(MetaList list);

    @Query("SELECT * FROM META_LIST")
    List<MetaList> getAll();
}
