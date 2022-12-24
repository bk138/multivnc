package com.coboltforge.dontmind.multivnc.db

import androidx.room.ColumnInfo
import androidx.room.Entity
import androidx.room.PrimaryKey
import kotlinx.serialization.Serializable

@Serializable
@Entity(tableName = "META_LIST")
data class MetaList(
        @JvmField
        @PrimaryKey(autoGenerate = true)
        @ColumnInfo(name = "_id")
        var id: Long = 0,

        @JvmField
        @ColumnInfo(name = "NAME")
        var name: String? = null
)