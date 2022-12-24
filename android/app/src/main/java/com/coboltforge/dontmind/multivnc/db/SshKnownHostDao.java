package com.coboltforge.dontmind.multivnc.db;

import androidx.room.Dao;
import androidx.room.Insert;
import androidx.room.Query;
import androidx.room.Update;

import java.util.List;

@Dao
public interface SshKnownHostDao {

    @Query("SELECT * FROM SSH_KNOWN_HOST WHERE _id = :id")
    SshKnownHost get(long id);

    @Query("SELECT * FROM SSH_KNOWN_HOST WHERE host = :host")
    SshKnownHost get(String host);

    @Query("SELECT * FROM SSH_KNOWN_HOST")
    List<SshKnownHost> getAll();

    @Insert
    long insert(SshKnownHost knownHost);

    @Update
    void update(SshKnownHost knownHost);

}
