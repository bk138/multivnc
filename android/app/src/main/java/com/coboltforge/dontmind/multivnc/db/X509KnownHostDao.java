package com.coboltforge.dontmind.multivnc.db;

import androidx.room.Dao;
import androidx.room.Insert;
import androidx.room.Query;
import androidx.room.Update;

import java.util.List;

@Dao
public interface X509KnownHostDao {

    @Query("SELECT * FROM X509_KNOWN_HOST WHERE _id = :id")
    X509KnownHost get(long id);

    @Query("SELECT * FROM X509_KNOWN_HOST WHERE host = :host")
    X509KnownHost get(String host);

    @Query("SELECT * FROM X509_KNOWN_HOST")
    List<X509KnownHost> getAll();

    @Insert
    long insert(X509KnownHost knownHost);

    @Update
    void update(X509KnownHost knownHost);

}
