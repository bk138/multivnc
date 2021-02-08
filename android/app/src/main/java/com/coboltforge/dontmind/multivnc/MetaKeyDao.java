package com.coboltforge.dontmind.multivnc;

import androidx.room.Dao;
import androidx.room.Insert;
import androidx.room.Query;

import java.util.List;

@Dao
public interface MetaKeyDao {

    @Query("SELECT * FROM META_KEY WHERE _id = :id")
    MetaKeyBean get(long id);

    @Query("SELECT * FROM META_KEY WHERE METALISTID = :metaListId")
    List<MetaKeyBean> getByMetaList(long metaListId );

    @Insert
    long insert(MetaKeyBean item);
}
