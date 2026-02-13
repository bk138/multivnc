package com.coboltforge.dontmind.multivnc.db

import androidx.room.ColumnInfo
import androidx.room.Entity
import androidx.room.PrimaryKey
import kotlinx.serialization.Serializable

@Serializable
@Entity(tableName = "X509_KNOWN_HOST")
data class X509KnownHost(
        @JvmField
        @PrimaryKey(autoGenerate = true)
        @ColumnInfo(name = "_id")
        val id: Long = 0,

        @JvmField
        @ColumnInfo(name = "HOST")
        val host: String,

        @JvmField
        @ColumnInfo(name = "FINGERPRINT")
        val fingerprint: ByteArray,
) {
        override fun equals(other: Any?): Boolean {
                if (this === other) return true
                if (javaClass != other?.javaClass) return false

                other as X509KnownHost

                if (id != other.id) return false
                if (host != other.host) return false
                if (!fingerprint.contentEquals(other.fingerprint)) return false

                return true
        }

        override fun hashCode(): Int {
                var result = id.hashCode()
                result = 31 * result + host.hashCode()
                result = 31 * result + fingerprint.contentHashCode()
                return result
        }

}