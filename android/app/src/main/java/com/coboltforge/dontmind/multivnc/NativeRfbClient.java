package com.coboltforge.dontmind.multivnc;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

/**
 * This is a thin wrapper around native RFB client.
 *
 *
 * -       +------------+                                    +----------+
 * -       | Public API |                                    | Listener |
 * -       +------------+                                    +-----A----+
 * -              |                                                |
 * -              |                                                |
 * -   JNI -------|------------------------------------------------|-----------
 * -              |                                                |
 * -              |                                                |
 * -      +-------v--------+       +--------------+       +--------v---------+
 * -      | Native Methods |------>| Libvncclient |<----->| Native Callbacks |
 * -      +----------------+       +--------------+       +------------------+
 */
public final class NativeRfbClient {

    /**
     * Value of the pointer to native 'rfbClient'.
     */
    private final long nativeRfbClientPtr;

    /**
     * RBF callback listener.
     */
    private final RfbListenerInterface callbackListener;

    //region Initialization
    static {
        System.loadLibrary("vncconn");
    }

    /**
     * Constructor.
     *
     * After successful construction, you must call cleanup() on this object
     * once you are done with it so that allocated resources can be freed.
     */
    NativeRfbClient(@NonNull RfbListenerInterface callbackListener) {
        this.callbackListener = callbackListener;
        this.nativeRfbClientPtr = nativeCreateClient();

        if (nativeRfbClientPtr == 0)
            throw new RuntimeException("Could not create native rfbClient!");
    }
    //endregion

    //region Public API

    /**
     * Returns pointer to the native rfbClient structure.
     */
    public long getNativeRfbClientPtr() {
        return nativeRfbClientPtr;
    }

    /**
     * Initializes VNC connection.
     * TODO: Add Repeater support
     *
     * @param host Server address
     * @param port Port number
     * @return true if initialization was successful
     */
    public boolean init(String host, int port) {
        return nativeInit(nativeRfbClientPtr, host, port);
    }

    /**
     * Waits for incoming server message, parses it and then invokes appropriate
     * callbacks.
     *
     * @return true if message was successfully handled or no message was received,
     * false otherwise.
     */
    public boolean processServerMessage() {
        return nativeProcessServerMessage(nativeRfbClientPtr);
    }

    /**
     * Sends Key event to remote server.
     *
     * @param key    Key code
     * @param isDown Whether it is an DOWN or UP event
     * @return true if sent successfully, false otherwise
     */
    public boolean sendKeyEvent(long key, boolean isDown) {
        return nativeSendKeyEvent(nativeRfbClientPtr, key, isDown);
    }

    /**
     * Sends pointer event to remote server.
     *
     * @param x    Horizontal pointer coordinate
     * @param y    Vertical pointer coordinate
     * @param mask Button mask to identify which button was pressed.
     * @return true if sent successfully, false otherwise
     */
    public boolean sendPointerEvent(int x, int y, int mask) {
        return nativeSendPointerEvent(nativeRfbClientPtr, x, y, mask);
    }

    /**
     * Sends text to remote desktop's clipboard.
     *
     * @param text Text to send
     * @return Whether sent successfully.
     */
    public boolean sendCutText(String text) {
        return nativeSendCutText(nativeRfbClientPtr, text);
    }

    /**
     * Returns width of frame buffer (in pixels).
     */
    public int getFrameBufferWidth() {
        return nativeGetFrameBufferWidth(nativeRfbClientPtr);
    }

    /**
     * Returns height of frame buffer (in pixels).
     */
    public int getFrameBufferHeight() {
        return nativeGetFrameBufferHeight(nativeRfbClientPtr);
    }

    /**
     * Returns name of the remote desktop.
     */
    public String getDesktopName() {
        return nativeGetDesktopName(nativeRfbClientPtr);
    }

    /**
     * Releases all resource (native & managed) currently held.
     * After cleanup, this object should not be used any more.
     */
    public void cleanup() {
        nativeCleanup(nativeRfbClientPtr);
    }
    //endregion

    //region Native Methods
    private native long nativeCreateClient();

    private native boolean nativeInit(long clientPtr, String host, int port);

    private native boolean nativeProcessServerMessage(long clientPtr);

    //TODO: This should return something like a 'ConnectionInfo' class which can contain
    //      Remote desktop name, framebuffer size, encoding, encryption status etc...
    //      OR
    //      Maybe we can create separate native function for each value and assemble
    //      'ConnectionInfo' in Java.
    //      OR
    //      Maybe we should return ConnectionInfo from init()
    //private native ??????? nativeGetConnectionInfo(long clientPtr);

    private native boolean nativeSendKeyEvent(long clientPtr, long key, boolean isDown);

    private native boolean nativeSendPointerEvent(long clientPtr, int x, int y, int mask);

    private native boolean nativeSendCutText(long clientPtr, String text);

    private native int nativeGetFrameBufferWidth(long clientPtr);

    private native int nativeGetFrameBufferHeight(long clientPtr);

    private native String nativeGetDesktopName(long clientPtr);

    private native void nativeCleanup(long clientPtr);
    //endregion

    //region Callbacks
    @Keep
    private String cbGetPassword() {
        return callbackListener.rfbGetPassword();
    }

    @Keep
    private RfbUserCredential cbGetCredential() {
        return callbackListener.rfbGetCredential();
    }

    @Keep
    private void cbBell() {
        callbackListener.rfbBell();
    }

    @Keep
    private void cbGotXCutText(String text) {
        callbackListener.rfbGotXCutText(text);
    }

    @Keep
    private boolean cbHandleCursorPos(int x, int y) {
        return callbackListener.rfbHandleCursorPos(x, y);
    }

    @Keep
    private void cbGotFrameBufferUpdate(int x, int y, int w, int h) {
        //TODO: 1. Decide if we really need this callback
        //         Ex: We may want to trigger a canvas redraw,
        //             but we already do that in cbFinishedFrameBufferUpdate
        //      2. Do we also need all these parameters?
        //      3. After deciding, remove OR implement it.
    }

    @Keep
    private void cbFinishedFrameBufferUpdate() {
        callbackListener.rfbFinishedFrameBufferUpdate();
    }

    /**
     * Interface for RFB callback listener.
     */
    interface RfbListenerInterface {
        String rfbGetPassword();

        RfbUserCredential rfbGetCredential();

        void rfbBell();

        void rfbGotXCutText(String text);

        void rfbFinishedFrameBufferUpdate();

        boolean rfbHandleCursorPos(int x, int y);
    }

    /**
     * This class is used for returning user credentials from callbacks.
     */
    public static class RfbUserCredential {
        public String username;
        public String password;
    }
    //endregion
}
