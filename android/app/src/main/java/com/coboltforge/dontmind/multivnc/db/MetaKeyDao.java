package com.coboltforge.dontmind.multivnc.db;

import androidx.room.Dao;
import androidx.room.Insert;
import androidx.room.Query;
import androidx.room.Update;

import java.util.List;

@Dao
public interface MetaKeyDao {

    @Query("SELECT * FROM META_KEY WHERE _id = :id")
    MetaKeyBean get(long id);

    @Query("SELECT * FROM META_KEY WHERE METALISTID = :metaListId")
    List<MetaKeyBean> getByMetaList(long metaListId );

    @Query("SELECT * FROM META_KEY")
    List<MetaKeyBean> getAll();

    @Insert
    long insert(MetaKeyBean item);

    @Update
    void update(MetaKeyBean item);
}
