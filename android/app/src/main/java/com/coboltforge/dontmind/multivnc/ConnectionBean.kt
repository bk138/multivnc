package com.coboltforge.dontmind.multivnc

import android.content.ContentValues
import androidx.room.ColumnInfo
import androidx.room.Entity
import androidx.room.PrimaryKey

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

) : Comparable<ConnectionBean> {

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


    /**********************************************************************************************
     * Serialization helpers
     * These are used to parcel ConnectionBean through Intents.
     *********************************************************************************************/

    companion object {
        const val GEN_FIELD__ID = "_id"
        const val GEN_FIELD_NICKNAME = "NICKNAME"
        const val GEN_FIELD_ADDRESS = "ADDRESS"
        const val GEN_FIELD_PORT = "PORT"
        const val GEN_FIELD_PASSWORD = "PASSWORD"
        const val GEN_FIELD_COLORMODEL = "COLORMODEL"
        const val GEN_FIELD_FORCEFULL = "FORCEFULL"
        const val GEN_FIELD_REPEATERID = "REPEATERID"
        const val GEN_FIELD_INPUTMODE = "INPUTMODE"
        const val GEN_FIELD_SCALEMODE = "SCALEMODE"
        const val GEN_FIELD_USELOCALCURSOR = "USELOCALCURSOR"
        const val GEN_FIELD_KEEPPASSWORD = "KEEPPASSWORD"
        const val GEN_FIELD_FOLLOWMOUSE = "FOLLOWMOUSE"
        const val GEN_FIELD_USEREPEATER = "USEREPEATER"
        const val GEN_FIELD_METALISTID = "METALISTID"
        const val GEN_FIELD_LAST_META_KEY_ID = "LAST_META_KEY_ID"
        const val GEN_FIELD_FOLLOWPAN = "FOLLOWPAN"
        const val GEN_FIELD_USERNAME = "USERNAME"
        const val GEN_FIELD_SECURECONNECTIONTYPE = "SECURECONNECTIONTYPE"
        const val GEN_FIELD_SHOWZOOMBUTTONS = "SHOWZOOMBUTTONS"
        const val GEN_FIELD_DOUBLE_TAP_ACTION = "DOUBLE_TAP_ACTION"
    }

    fun Gen_getValues(): ContentValues {
        val values = ContentValues()
        values.put(GEN_FIELD__ID, java.lang.Long.toString(id))
        values.put(GEN_FIELD_NICKNAME, nickname)
        values.put(GEN_FIELD_ADDRESS, address)
        values.put(GEN_FIELD_PORT, Integer.toString(port))
        values.put(GEN_FIELD_PASSWORD, password)
        values.put(GEN_FIELD_COLORMODEL, colorModel)
        values.put(GEN_FIELD_FORCEFULL, java.lang.Long.toString(forceFull))
        values.put(GEN_FIELD_REPEATERID, repeaterId)
        values.put(GEN_FIELD_INPUTMODE, inputMode)
        values.put(GEN_FIELD_SCALEMODE, scalemode)
        values.put(GEN_FIELD_USELOCALCURSOR, if (useLocalCursor) "1" else "0")
        values.put(GEN_FIELD_KEEPPASSWORD, if (keepPassword) "1" else "0")
        values.put(GEN_FIELD_FOLLOWMOUSE, if (followMouse) "1" else "0")
        values.put(GEN_FIELD_USEREPEATER, if (useRepeater) "1" else "0")
        values.put(GEN_FIELD_METALISTID, java.lang.Long.toString(metaListId))
        values.put(GEN_FIELD_LAST_META_KEY_ID, java.lang.Long.toString(lastMetaKeyId))
        values.put(GEN_FIELD_FOLLOWPAN, if (followPan) "1" else "0")
        values.put(GEN_FIELD_USERNAME, userName)
        values.put(GEN_FIELD_SECURECONNECTIONTYPE, secureConnectionType)
        values.put(GEN_FIELD_SHOWZOOMBUTTONS, if (showZoomButtons) "1" else "0")
        values.put(GEN_FIELD_DOUBLE_TAP_ACTION, doubleTapAction)
        return values
    }

    /**
     * Populate one instance from a ContentValues
     */
    fun Gen_populate(values: ContentValues) {
        id = values.getAsLong(GEN_FIELD__ID)
        nickname = values.getAsString(GEN_FIELD_NICKNAME)
        address = values.getAsString(GEN_FIELD_ADDRESS)
        port = values.getAsInteger(GEN_FIELD_PORT)
        password = values.getAsString(GEN_FIELD_PASSWORD)
        colorModel = values.getAsString(GEN_FIELD_COLORMODEL)
        forceFull = values.getAsLong(GEN_FIELD_FORCEFULL)
        repeaterId = values.getAsString(GEN_FIELD_REPEATERID)
        inputMode = values.getAsString(GEN_FIELD_INPUTMODE)
        scalemode = values.getAsString(GEN_FIELD_SCALEMODE)
        useLocalCursor = values.getAsInteger(GEN_FIELD_USELOCALCURSOR) != 0
        keepPassword = values.getAsInteger(GEN_FIELD_KEEPPASSWORD) != 0
        followMouse = values.getAsInteger(GEN_FIELD_FOLLOWMOUSE) != 0
        useRepeater = values.getAsInteger(GEN_FIELD_USEREPEATER) != 0
        metaListId = values.getAsLong(GEN_FIELD_METALISTID)
        lastMetaKeyId = values.getAsLong(GEN_FIELD_LAST_META_KEY_ID)
        followPan = values.getAsInteger(GEN_FIELD_FOLLOWPAN) != 0
        userName = values.getAsString(GEN_FIELD_USERNAME)
        secureConnectionType = values.getAsString(GEN_FIELD_SECURECONNECTIONTYPE)
        showZoomButtons = values.getAsInteger(GEN_FIELD_SHOWZOOMBUTTONS) != 0
        doubleTapAction = values.getAsString(GEN_FIELD_DOUBLE_TAP_ACTION)
    }
}