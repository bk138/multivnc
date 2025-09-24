package com.coboltforge.dontmind.multivnc

import android.view.KeyEvent

/**
 * Maps Android KeyEvent codes to XT (PC/AT) key codes for extended key events.
 *
 * XT key codes are required by the QEMU Extended Key Event protocol.
 * Android's getScanCode() does not return XT codes, so we need explicit mapping.
 *
 * For 2-byte XT codes (like arrow keys), we encode them as single integers
 * where the high byte represents the 0xe0 prefix.
 *
 * See https://github.com/rfbproto/rfbproto/blob/master/rfbproto.rst#qemu-extended-key-event-message
 * for details.
 */
object AndroidKeyToXt {
    /**
     * Maps Android KeyEvent keyCode to XT key code.
     * @param androidKeyCode KeyEvent keyCode from Android
     * @return XT key code, or 0 if unknown/unsupported
     */
    @JvmStatic
    fun getXtKeycode(androidKeyCode: Int): Int {
        return when (androidKeyCode) {
            // Function keys F1-F12
            KeyEvent.KEYCODE_F1 -> 0x3b
            KeyEvent.KEYCODE_F2 -> 0x3c
            KeyEvent.KEYCODE_F3 -> 0x3d
            KeyEvent.KEYCODE_F4 -> 0x3e
            KeyEvent.KEYCODE_F5 -> 0x3f
            KeyEvent.KEYCODE_F6 -> 0x40
            KeyEvent.KEYCODE_F7 -> 0x41
            KeyEvent.KEYCODE_F8 -> 0x42
            KeyEvent.KEYCODE_F9 -> 0x43
            KeyEvent.KEYCODE_F10 -> 0x44
            KeyEvent.KEYCODE_F11 -> 0x57
            KeyEvent.KEYCODE_F12 -> 0x58

            // Arrow keys (2-byte XT codes with 0xe0 prefix)
            KeyEvent.KEYCODE_DPAD_LEFT -> 0xe04b   // Left Arrow: 0xe0 0x4b
            KeyEvent.KEYCODE_DPAD_UP -> 0xe048     // Up Arrow: 0xe0 0x48
            KeyEvent.KEYCODE_DPAD_RIGHT -> 0xe04d  // Right Arrow: 0xe0 0x4d
            KeyEvent.KEYCODE_DPAD_DOWN -> 0xe050   // Down Arrow: 0xe0 0x50

            // Navigation keys (2-byte XT codes)
            KeyEvent.KEYCODE_MOVE_HOME -> 0xe047   // Home: 0xe0 0x47
            KeyEvent.KEYCODE_MOVE_END -> 0xe04f    // End: 0xe0 0x4f
            KeyEvent.KEYCODE_PAGE_UP -> 0xe049     // Page Up: 0xe0 0x49
            KeyEvent.KEYCODE_PAGE_DOWN -> 0xe051   // Page Down: 0xe0 0x51
            KeyEvent.KEYCODE_INSERT -> 0xe052      // Insert: 0xe0 0x52
            KeyEvent.KEYCODE_FORWARD_DEL -> 0xe053 // Delete: 0xe0 0x53

            // Common keys
            KeyEvent.KEYCODE_ESCAPE -> 0x01        // Escape
            KeyEvent.KEYCODE_TAB -> 0x0f           // Tab
            KeyEvent.KEYCODE_DEL -> 0x0e           // Backspace
            KeyEvent.KEYCODE_ENTER -> 0x1c         // Enter
            KeyEvent.KEYCODE_SPACE -> 0x39         // Space

            // Modifier keys
            KeyEvent.KEYCODE_SHIFT_LEFT -> 0x2a    // Left Shift
            KeyEvent.KEYCODE_SHIFT_RIGHT -> 0x36   // Right Shift
            KeyEvent.KEYCODE_CTRL_LEFT -> 0x1d     // Left Ctrl
            KeyEvent.KEYCODE_CTRL_RIGHT -> 0xe01d  // Right Ctrl: 0xe0 0x1d
            KeyEvent.KEYCODE_ALT_LEFT -> 0x38      // Left Alt
            KeyEvent.KEYCODE_ALT_RIGHT -> 0xe038   // Right Alt (AltGr): 0xe0 0x38

            // Alphanumeric keys
            KeyEvent.KEYCODE_A -> 0x1e
            KeyEvent.KEYCODE_B -> 0x30
            KeyEvent.KEYCODE_C -> 0x2e
            KeyEvent.KEYCODE_D -> 0x20
            KeyEvent.KEYCODE_E -> 0x12
            KeyEvent.KEYCODE_F -> 0x21
            KeyEvent.KEYCODE_G -> 0x22
            KeyEvent.KEYCODE_H -> 0x23
            KeyEvent.KEYCODE_I -> 0x17
            KeyEvent.KEYCODE_J -> 0x24
            KeyEvent.KEYCODE_K -> 0x25
            KeyEvent.KEYCODE_L -> 0x26
            KeyEvent.KEYCODE_M -> 0x32
            KeyEvent.KEYCODE_N -> 0x31
            KeyEvent.KEYCODE_O -> 0x18
            KeyEvent.KEYCODE_P -> 0x19
            KeyEvent.KEYCODE_Q -> 0x10
            KeyEvent.KEYCODE_R -> 0x13
            KeyEvent.KEYCODE_S -> 0x1f
            KeyEvent.KEYCODE_T -> 0x14
            KeyEvent.KEYCODE_U -> 0x16
            KeyEvent.KEYCODE_V -> 0x2f
            KeyEvent.KEYCODE_W -> 0x11
            KeyEvent.KEYCODE_X -> 0x2d
            KeyEvent.KEYCODE_Y -> 0x15
            KeyEvent.KEYCODE_Z -> 0x2c

            // Number row keys
            KeyEvent.KEYCODE_0 -> 0x0b
            KeyEvent.KEYCODE_1 -> 0x02
            KeyEvent.KEYCODE_2 -> 0x03
            KeyEvent.KEYCODE_3 -> 0x04
            KeyEvent.KEYCODE_4 -> 0x05
            KeyEvent.KEYCODE_5 -> 0x06
            KeyEvent.KEYCODE_6 -> 0x07
            KeyEvent.KEYCODE_7 -> 0x08
            KeyEvent.KEYCODE_8 -> 0x09
            KeyEvent.KEYCODE_9 -> 0x0a

            // Symbol keys
            KeyEvent.KEYCODE_MINUS -> 0x0c         // -
            KeyEvent.KEYCODE_EQUALS -> 0x0d        // =
            KeyEvent.KEYCODE_LEFT_BRACKET -> 0x1a  // [
            KeyEvent.KEYCODE_RIGHT_BRACKET -> 0x1b // ]
            KeyEvent.KEYCODE_SEMICOLON -> 0x27     // ;
            KeyEvent.KEYCODE_APOSTROPHE -> 0x28    // '
            KeyEvent.KEYCODE_GRAVE -> 0x29         // `
            KeyEvent.KEYCODE_BACKSLASH -> 0x2b     // \
            KeyEvent.KEYCODE_COMMA -> 0x33         // ,
            KeyEvent.KEYCODE_PERIOD -> 0x34        // .
            KeyEvent.KEYCODE_SLASH -> 0x35         // /

            // Numeric keypad (2-byte codes for most)
            KeyEvent.KEYCODE_NUMPAD_0 -> 0x52
            KeyEvent.KEYCODE_NUMPAD_1 -> 0x4f
            KeyEvent.KEYCODE_NUMPAD_2 -> 0x50
            KeyEvent.KEYCODE_NUMPAD_3 -> 0x51
            KeyEvent.KEYCODE_NUMPAD_4 -> 0x4b
            KeyEvent.KEYCODE_NUMPAD_5 -> 0x4c
            KeyEvent.KEYCODE_NUMPAD_6 -> 0x4d
            KeyEvent.KEYCODE_NUMPAD_7 -> 0x47
            KeyEvent.KEYCODE_NUMPAD_8 -> 0x48
            KeyEvent.KEYCODE_NUMPAD_9 -> 0x49
            KeyEvent.KEYCODE_NUMPAD_DIVIDE -> 0xe035    // Keypad /: 0xe0 0x35
            KeyEvent.KEYCODE_NUMPAD_MULTIPLY -> 0x37    // Keypad *
            KeyEvent.KEYCODE_NUMPAD_SUBTRACT -> 0x4a    // Keypad -
            KeyEvent.KEYCODE_NUMPAD_ADD -> 0x4e         // Keypad +
            KeyEvent.KEYCODE_NUMPAD_ENTER -> 0xe01c     // Keypad Enter: 0xe0 0x1c
            KeyEvent.KEYCODE_NUMPAD_DOT -> 0x53         // Keypad .

            // Lock keys
            KeyEvent.KEYCODE_CAPS_LOCK -> 0x3a     // Caps Lock
            KeyEvent.KEYCODE_NUM_LOCK -> 0x45      // Num Lock
            KeyEvent.KEYCODE_SCROLL_LOCK -> 0x46   // Scroll Lock

            // System keys
            KeyEvent.KEYCODE_BREAK -> 0xe046       // Pause/Break: 0xe0 0x46
            KeyEvent.KEYCODE_SYSRQ -> 0x54         // Print Screen/SysRq

            // Unknown or unsupported key
            else -> 0
        }
    }

}