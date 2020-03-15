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
 * -      | Native Methods |------>| LibVNCClient |<----->| Native Callbacks |
 * -      +----------------+       +--------------+       +------------------+
 */
public final class NativeRfbClient {

    /**
     * Value of the pointer to native 'rfbClient'.
     */
    private final long nativeRfbClientPtr;

    /**
     * RFB callback listener.
     */
    private final ICallbackListener callbackListener;

    /**
     * Holds information about the current connection.
     */
    private ConnectionInfo connectionInfo;

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
    NativeRfbClient(@NonNull ICallbackListener callbackListener) {
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
        connectionInfo = null;
        return nativeInit(nativeRfbClientPtr, host, port);
    }

    /**
     * Waits for incoming server message, parses it and then invokes appropriate
     * callbacks.
     *
     * @param uSecTimeout Timeout in microseconds.
     * @return true if message was successfully handled or no message was received within timeout,
     * false otherwise.
     */
    public boolean processServerMessage(int uSecTimeout) {
        return nativeProcessServerMessage(nativeRfbClientPtr, uSecTimeout);
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
     * Sends a request for full frame buffer update to remote server.
     *
     * @return Whether sent successfully
     */
    public boolean sendFrameBufferUpdateRequest() {
        return sendFrameBufferUpdateRequest(
                0,
                0,
                getConnectionInfo().frameWidth,
                getConnectionInfo().frameHeight,
                false
        );
    }

    /**
     * Sends frame buffer update request for given region.
     *
     * @param x           Horizontal start position
     * @param y           Vertical start position
     * @param w           Width of the region
     * @param h           Height of the region
     * @param incremental True if only updated parts of the region should be returned
     * @return Whether sent successfully
     */
    public boolean sendFrameBufferUpdateRequest(int x, int y, int w, int h, boolean incremental) {
        return nativeSendFrameBufferUpdateRequest(nativeRfbClientPtr, x, y, w, h, incremental);
    }

    /**
     * Releases all resource (native & managed) currently held.
     * After cleanup, this object should not be used any more.
     */
    public void cleanup() {
        nativeCleanup(nativeRfbClientPtr);
    }

    /**
     * Returns information about current connection.
     *
     * @return
     */
    public ConnectionInfo getConnectionInfo() {
        return getConnectionInfo(false);
    }

    /**
     * Returns information about current connection.
     *
     * @param refresh Whether information should be reloaded from native rfbClient.
     */
    public ConnectionInfo getConnectionInfo(boolean refresh) {
        if (connectionInfo == null || refresh) {
            connectionInfo = nativeGetConnectionInfo(nativeRfbClientPtr);
        }
        return connectionInfo;
    }

    /**
     * This class is used for representing information about the current connection.
     *
     * Note: This class is instantiates by the native code. Any change in fields & constructor
     * arguments should be synchronized with native side.
     *
     * TODO: Should we make this a standalone class?
     * TODO: Add info about encoding etc.
     */
    public static final class ConnectionInfo {
        public final String desktopName;
        public final int frameWidth;
        public final int frameHeight;
        public final boolean isEncrypted;

        public ConnectionInfo(String desktopName, int frameWidth, int frameHeight, boolean isEncrypted) {
            this.desktopName = desktopName;
            this.frameWidth = frameWidth;
            this.frameHeight = frameHeight;
            this.isEncrypted = isEncrypted;
        }
    }
    //endregion

    //region Native Methods
    private native long nativeCreateClient();

    private native boolean nativeInit(long clientPtr, String host, int port);

    private native boolean nativeProcessServerMessage(long clientPtr, int uSecTimeout);

    private native boolean nativeSendKeyEvent(long clientPtr, long key, boolean isDown);

    private native boolean nativeSendPointerEvent(long clientPtr, int x, int y, int mask);

    private native boolean nativeSendCutText(long clientPtr, String text);

    private native boolean nativeSendFrameBufferUpdateRequest(long clientPtr, int x, int y, int w, int h, boolean incremental);

    private native ConnectionInfo nativeGetConnectionInfo(long clientPtr);

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
    private void cbFinishedFrameBufferUpdate() {
        callbackListener.rfbFinishedFrameBufferUpdate();
    }

    /**
     * Interface for RFB callback listener.
     */
    interface ICallbackListener {
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
