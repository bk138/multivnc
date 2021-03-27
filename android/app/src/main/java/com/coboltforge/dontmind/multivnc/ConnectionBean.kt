package com.coboltforge.dontmind.multivnc

import android.os.Parcelable
import androidx.room.ColumnInfo
import androidx.room.Entity
import androidx.room.PrimaryKey
import kotlinx.parcelize.Parcelize

@Parcelize
@Entity(tableName = "CONNECTION_BEAN")
data class ConnectionBean(
        @JvmField
        @PrimaryKey(autoGenerate = true)
        @ColumnInfo(name = "_id")
        var id: Long = 0,

        @JvmField
        @ColumnInfo(name = "NICKNAME")
        var nickname: String? = "",

        @JvmField
        @ColumnInfo(name = "ADDRESS")
        var address: String? = "",

        @JvmField
        @ColumnInfo(name = "PORT")
        var port: Int = 5900,

        @JvmField
        @ColumnInfo(name = "PASSWORD")
        var password: String? = "",

        @JvmField
        @ColumnInfo(name = "COLORMODEL")
        var colorModel: String? = COLORMODEL.C24bit.nameString(),

        @JvmField
        @ColumnInfo(name = "FORCEFULL")
        var forceFull: Long = 0,

        @JvmField
        @ColumnInfo(name = "REPEATERID")
        var repeaterId: String? = "",

        @JvmField
        @ColumnInfo(name = "INPUTMODE")
        var inputMode: String? = null,

        @JvmField
        @ColumnInfo(name = "SCALEMODE")
        var scalemode: String? = null,

        @JvmField
        @ColumnInfo(name = "USELOCALCURSOR")
        var useLocalCursor: Boolean = false,

        @JvmField
        @ColumnInfo(name = "KEEPPASSWORD")
        var keepPassword: Boolean = true,

        @JvmField
        @ColumnInfo(name = "FOLLOWMOUSE")
        var followMouse: Boolean = true,

        @JvmField
        @ColumnInfo(name = "USEREPEATER")
        var useRepeater: Boolean = false,

        @JvmField
        @ColumnInfo(name = "METALISTID")
        var metaListId: Long = 1,

        @JvmField
        @ColumnInfo(name = "LAST_META_KEY_ID")
        var lastMetaKeyId: Long = 0,

        @JvmField
        @ColumnInfo(name = "FOLLOWPAN", defaultValue = "0")
        var followPan: Boolean = false,

        @JvmField
        @ColumnInfo(name = "USERNAME")
        var userName: String? = "",

        @JvmField
        @ColumnInfo(name = "SECURECONNECTIONTYPE")
        var secureConnectionType: String? = null,

        @JvmField
        @ColumnInfo(name = "SHOWZOOMBUTTONS", defaultValue = "1")
        var showZoomButtons: Boolean = false,

        @JvmField
        @ColumnInfo(name = "DOUBLE_TAP_ACTION")
        var doubleTapAction: String? = null

) : Comparable<ConnectionBean>, Parcelable {

    override fun toString(): String {
        return "$id $nickname: $address, port $port"
    }

    override fun compareTo(other: ConnectionBean): Int {
        var result = nickname!!.compareTo(other.nickname!!)
        if (result == 0) {
            result = address!!.compareTo(other.address!!)
            if (result == 0) {
                result = port - other.port
            }
        }
        return result
    }

    /**
     * parse host:port or [host]:port and split into address and port fields
     * @param hostport_str
     * @return true if there was a port, false if not
     */
    fun parseHostPort(hostport_str: String): Boolean {
        val nr_colons = hostport_str.replace("[^:]".toRegex(), "").length
        val nr_endbrackets = hostport_str.replace("[^]]".toRegex(), "").length

        if (nr_colons == 1) { // IPv4
            val p = hostport_str.substring(hostport_str.indexOf(':') + 1)
            try {
                port = p.toInt()
            } catch (e: Exception) {
            }
            address = hostport_str.substring(0, hostport_str.indexOf(':'))
            return true
        }
        if (nr_colons > 1 && nr_endbrackets == 1) {
            val p = hostport_str.substring(hostport_str.indexOf(']') + 2) // it's [addr]:port
            try {
                port = p.toInt()
            } catch (e: Exception) {
            }
            address = hostport_str.substring(0, hostport_str.indexOf(']') + 1)
            return true
        }
        return false
    }

    companion object {
        //These are used by ConnectionListActivity
        const val GEN_FIELD_NICKNAME = "NICKNAME"
        const val GEN_FIELD_ADDRESS = "ADDRESS"
        const val GEN_FIELD_PORT = "PORT"
        const val GEN_FIELD_REPEATERID = "REPEATERID"
    }
}