package com.coboltforge.dontmind.multivnc

import android.view.KeyEvent
import androidx.room.ColumnInfo
import androidx.room.Entity
import androidx.room.Ignore
import androidx.room.PrimaryKey
import kotlinx.serialization.Serializable
import java.util.*

@Serializable
@Entity(tableName = "META_KEY")
data class MetaKeyBean(
        @JvmField
        @PrimaryKey(autoGenerate = true)
        @ColumnInfo(name = "_id")
        var id: Long = 0,

        @JvmField
        @ColumnInfo(name = "METALISTID")
        var metaListId: Long = 0,

        @JvmField
        @ColumnInfo(name = "KEYDESC")
        var keyDesc: String? = null,

        @JvmField
        @ColumnInfo(name = "METAFLAGS")
        var metaFlags: Int = 0,

        @JvmField
        @ColumnInfo(name = "MOUSECLICK")
        var isMouseClick: Boolean = false,

        @JvmField
        @ColumnInfo(name = "MOUSEBUTTONS")
        var mouseButtons: Int = 0,

        @JvmField
        @ColumnInfo(name = "KEYSYM")
        var keySym: Int = 0,

        @JvmField
        @ColumnInfo(name = "SHORTCUT")
        var shortcut: String? = null

) : Comparable<MetaKeyBean> {

    companion object {
        @JvmField
        val allKeys = ArrayList<MetaKeyBase>()

        @JvmField
        var allKeysNames: Array<String?>

        @JvmField
        val keysByKeyCode = HashMap<Int, MetaKeyBase>()

        @JvmField
        val keysByMouseButton = HashMap<Int, MetaKeyBase>()

        @JvmField
        val keysByKeySym = HashMap<Int, MetaKeyBase>()

        var keyCtrlAltDel: MetaKeyBean? = null

        @JvmField
        var keyArrowLeft: MetaKeyBean? = null

        @JvmField
        var keyArrowRight: MetaKeyBean? = null

        @JvmField
        var keyArrowUp: MetaKeyBean? = null

        @JvmField
        var keyArrowDown: MetaKeyBean? = null

        init {
            allKeys.add(MetaKeyBase("Hangul", 0xff31))
            allKeys.add(MetaKeyBase("Hangul_Start", 0xff32))
            allKeys.add(MetaKeyBase("Hangul_End", 0xff33))
            allKeys.add(MetaKeyBase("Hangul_Hanja", 0xff34))
            allKeys.add(MetaKeyBase("Kana_Shift", 0xff2e))
            allKeys.add(MetaKeyBase("Right_Alt", 0xffea))

            allKeys.add(MetaKeyBase(VNCConn.MOUSE_BUTTON_LEFT, "Mouse Left"))
            allKeys.add(MetaKeyBase(VNCConn.MOUSE_BUTTON_MIDDLE, "Mouse Middle"))
            allKeys.add(MetaKeyBase(VNCConn.MOUSE_BUTTON_RIGHT, "Mouse Right"))
            allKeys.add(MetaKeyBase(VNCConn.MOUSE_BUTTON_SCROLL_DOWN, "Mouse Scroll Down"))
            allKeys.add(MetaKeyBase(VNCConn.MOUSE_BUTTON_SCROLL_UP, "Mouse Scroll Up"))

            allKeys.add(MetaKeyBase("Home", 0xFF50))
            allKeys.add(MetaKeyBase("Arrow Left", 0xFF51))
            allKeys.add(MetaKeyBase("Arrow Up", 0xFF52))
            allKeys.add(MetaKeyBase("Arrow Right", 0xFF53))
            allKeys.add(MetaKeyBase("Arrow Down", 0xFF54))
            allKeys.add(MetaKeyBase("Page Up", 0xFF55))
            allKeys.add(MetaKeyBase("Page Down", 0xFF56))
            allKeys.add(MetaKeyBase("End", 0xFF57))
            allKeys.add(MetaKeyBase("Insert", 0xFF63))
            allKeys.add(MetaKeyBase("Delete", 0xFFFF, KeyEvent.KEYCODE_DEL))
            allKeys.add(MetaKeyBase("Num Lock", 0xFF7F))
            allKeys.add(MetaKeyBase("Break", 0xFF6b))
            allKeys.add(MetaKeyBase("Scroll Lock", 0xFF14))
            allKeys.add(MetaKeyBase("Print Scrn", 0xFF15))
            allKeys.add(MetaKeyBase("Escape", 0xFF1B))
            allKeys.add(MetaKeyBase("Enter", 0xFF0D, KeyEvent.KEYCODE_ENTER))
            allKeys.add(MetaKeyBase("Tab", 0xFF09, KeyEvent.KEYCODE_TAB))
            allKeys.add(MetaKeyBase("BackSpace", 0xFF08))
            allKeys.add(MetaKeyBase("Space", 0x020, KeyEvent.KEYCODE_SPACE))

            val sb = StringBuilder(" ")
            for (i in 0..25) {
                sb.setCharAt(0, ('A'.toInt() + i).toChar())
                allKeys.add(MetaKeyBase(sb.toString(), 'a'.toInt() + i, KeyEvent.KEYCODE_A + i))
            }

            for (i in 0..9) {
                sb.setCharAt(0, ('0'.toInt() + i).toChar())
                allKeys.add(MetaKeyBase(sb.toString(), '0'.toInt() + i, KeyEvent.KEYCODE_0 + i))
            }

            for (i in 0..11) {
                sb.setLength(0)
                sb.append('F')
                if (i < 9)
                    sb.append(' ')
                sb.append((i + 1).toString())
                allKeys.add(MetaKeyBase(sb.toString(), 0xFFBE + i))
            }

            Collections.sort(allKeys)
            allKeysNames = arrayOfNulls(allKeys.size)
            for (i in allKeysNames.indices) {
                val b = allKeys[i]
                allKeysNames[i] = b.name
                if (b.isKeyEvent)
                    keysByKeyCode[b.keyEvent] = b
                if (b.isMouse)
                    keysByMouseButton[b.mouseButtons] = b
                else
                    keysByKeySym[b.keySym] = b
            }
            keyCtrlAltDel = MetaKeyBean(0, VNCConn.CTRL_MASK or VNCConn.ALT_MASK, keysByKeyCode[KeyEvent.KEYCODE_DEL])
            keyArrowLeft = MetaKeyBean(0, 0, keysByKeySym[0xFF51])
            keyArrowUp = MetaKeyBean(0, 0, keysByKeySym[0xFF52])
            keyArrowRight = MetaKeyBean(0, 0, keysByKeySym[0xFF53])
            keyArrowDown = MetaKeyBean(0, 0, keysByKeySym[0xFF54])
        }
    }


    @Ignore
    private var _regenDesc = false


    constructor(toCopy: MetaKeyBean) : this() {
        _regenDesc = true
        if (toCopy.isMouseClick)
            setMouseButtons(toCopy.mouseButtons)
        else
            setKeySym(toCopy.keySym)
        metaListId = toCopy.metaListId
        setMetaFlags(toCopy.metaFlags)
    }

    constructor(listId: Long, metaFlags: Int, base: MetaKeyBase?) : this() {
        metaListId = listId
        setKeyBase(base)
        setMetaFlags(metaFlags)
        _regenDesc = true
    }

    fun getKeyDesc(): String? {
        if (_regenDesc) {
            synchronized(this) {
                if (_regenDesc) {
                    val sb = StringBuilder()
                    val meta = metaFlags
                    if (0 != meta and VNCConn.SHIFT_MASK) {
                        sb.append("Shift")
                    }
                    if (0 != meta and VNCConn.CTRL_MASK) {
                        if (sb.isNotEmpty())
                            sb.append('-')
                        sb.append("Ctrl")
                    }
                    if (0 != meta and VNCConn.ALT_MASK) {
                        if (sb.isNotEmpty())
                            sb.append('-')
                        sb.append("Alt")
                    }
                    if (0 != meta and VNCConn.SUPER_MASK) {
                        if (sb.isNotEmpty())
                            sb.append('-')
                        sb.append("Super")
                    }
                    if (sb.isNotEmpty())
                        sb.append(' ')
                    val base = if (isMouseClick)
                        keysByMouseButton[mouseButtons]
                    else
                        keysByKeySym[keySym]
                    sb.append(base!!.name)
                    setKeyDesc(sb.toString())
                }
            }
        }
        return keyDesc
    }

    fun setKeyDesc(arg_keyDesc: String?) {
        keyDesc = arg_keyDesc
        _regenDesc = false
    }

    fun setKeySym(arg_keySym: Int) {
        if (arg_keySym != keySym || isMouseClick) {
            isMouseClick = false
            _regenDesc = true
            keySym = arg_keySym
        }
    }

    fun setMetaFlags(arg_metaFlags: Int) {
        if (arg_metaFlags != metaFlags) {
            _regenDesc = true
            metaFlags = arg_metaFlags
        }
    }

    fun setMouseButtons(arg_mouseButtons: Int) {
        if (arg_mouseButtons != mouseButtons || !isMouseClick) {
            isMouseClick = true
            _regenDesc = true
            mouseButtons = arg_mouseButtons
        }
    }

    fun setKeyBase(base: MetaKeyBase?) {
        if (base!!.isMouse) {
            setMouseButtons(base.mouseButtons)
        } else {
            setKeySym(base.keySym)
        }
    }

    override fun equals(other: Any?): Boolean {
        return if (other is MetaKeyBean) {
            getKeyDesc() == other.getKeyDesc()
        } else false
    }

    override fun compareTo(other: MetaKeyBean): Int {
        return getKeyDesc()!!.compareTo(other.getKeyDesc()!!)
    }
}