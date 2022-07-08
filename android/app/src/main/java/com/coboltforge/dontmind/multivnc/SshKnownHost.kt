package com.coboltforge.dontmind.multivnc

import androidx.room.ColumnInfo
import androidx.room.Entity
import androidx.room.PrimaryKey
import kotlinx.serialization.Serializable

@Serializable
@Entity(tableName = "SSH_KNOWN_HOST")
data class SshKnownHost(
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

                other as SshKnownHost

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