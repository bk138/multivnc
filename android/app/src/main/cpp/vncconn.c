/*
   vncconn.c: VNCConn.java native backend.
   This file is part of MultiVNC, a Multicast-enabled cross-platform
   VNC viewer.
   Copyright (C) 2009 - 2019 Christian Beier <dontmind@freeshell.org>
   MultiVNC is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   MultiVNC is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <jni.h>
#include <android/log.h>
#include <errno.h>
#include <rfb/rfbclient.h>

#define TAG "VNCConn-native"

/* id for the managed VNCConn */
#define VNCCONN_OBJ_ID (void*)1
#define VNCCONN_ENV_ID (void*)2

/* id for the managed NativeRfbClient  */
#define JAVA_RFBCLIENT_OBJ_ID (void*)10
#define JAVA_RFBCLIENT_ENV_ID (void*)11


/*
 * PixelFormat defaults.
 * Seems 8,3,4 and 5,3,2 are possible with rfbGetClient().
 */
#define BITSPERSAMPLE 8
#define SAMPLESPERPIXEL 3
#define BYTESPERPIXEL 4


/*
 * Modeled after rfbDefaultClientLog:
 *  - with Android log functions
 *  - without time stamping as the Android logging does this already
 * There's no per-connection log since we cannot find out which client
 * called the logger function :-(
 */
static void logcat_logger(const char *format, ...)
{
    va_list args;

    if(!rfbEnableClientLogging)
        return;

    va_start(args, format);
    __android_log_vprint(ANDROID_LOG_INFO, TAG, format, args);
    va_end(args);
}


static void log_obj_tostring(JNIEnv *env, jobject obj, const char *format, ...) {
    jclass cls = (*env)->GetObjectClass(env, obj);
    jmethodID mid = (*env)->GetMethodID(env, cls, "toString", "()Ljava/lang/String;");
    jstring jStr = (*env)->CallObjectMethod(env, obj, mid);
    const char *cStr = (*env)->GetStringUTFChars(env, jStr, NULL);
    va_list args;

    /* prefix format string with result of toString() */
    const size_t format_buf_len = 1024;
    char *format_buf[format_buf_len];
    snprintf((char*)format_buf, format_buf_len, "%s: %s", cStr, format);

    va_start(args, format);
    __android_log_vprint(ANDROID_LOG_INFO, TAG, (char*)format_buf, args);
    va_end(args);

    (*env)->ReleaseStringUTFChars(env, jStr, cStr);
}


/*
 * The VM calls JNI_OnLoad when the native library is loaded (for example, through System.loadLibrary).
 * JNI_OnLoad must return the JNI version needed by the native library.
 * We use this to wire up LibVNCClient logging to logcat.
 */
JNIEXPORT jint JNI_OnLoad(JavaVM __unused * vm, void __unused * reserved) {

    __android_log_print(ANDROID_LOG_DEBUG, TAG, "libvncconn loading\n");

    rfbClientLog = rfbClientErr = logcat_logger;

    return JNI_VERSION_1_6;
}


/**
 * Get the managed VNCConn's rfbClient.
 */
static rfbClient* getRfbClient(JNIEnv *env, jobject conn) {
    rfbClient* cl = NULL;
    jclass cls = (*env)->GetObjectClass(env, conn);
    jfieldID fid = (*env)->GetFieldID(env, cls, "rfbClient", "J");
    if (fid == 0)
        return NULL;

    cl = (rfbClient*)(long)(*env)->GetLongField(env, conn, fid);

    return cl;
}

/**
 * Set the managed VNCConn's rfbClient.
 */
static jboolean setRfbClient(JNIEnv *env, jobject conn, rfbClient* cl) {
    jclass cls = (*env)->GetObjectClass(env, conn);
    jfieldID fid = (*env)->GetFieldID(env, cls, "rfbClient", "J");
    if (fid == 0)
        return JNI_FALSE;

    (*env)->SetLongField(env, conn, fid, (long)cl);

    return JNI_TRUE;
}


static void onFramebufferUpdateFinished(rfbClient* client)
{
    jobject obj = rfbClientGetClientData(client, VNCCONN_OBJ_ID);
    JNIEnv *env = rfbClientGetClientData(client, VNCCONN_ENV_ID);

    jclass cls = (*env)->GetObjectClass(env, obj);
    jmethodID mid = (*env)->GetMethodID(env, cls, "onFramebufferUpdateFinished", "()V");
    (*env)->CallVoidMethod(env, obj, mid);
}


/**
 * Allocates and sets up the VNCConn's rfbClient.
 * @param env
 * @param obj
 * @return
 */
static jboolean setupClient(JNIEnv *env, jobject obj) {

    log_obj_tostring(env, obj, "setupClient()");

    if(getRfbClient(env, obj)) { /* already set up */
        log_obj_tostring(env, obj, "setupClient() already done");
        return JNI_FALSE;
    }

    rfbClient *cl = rfbGetClient(BITSPERSAMPLE, SAMPLESPERPIXEL, BYTESPERPIXEL);

    // set callbacks
    cl->FinishedFrameBufferUpdate = onFramebufferUpdateFinished;

    setRfbClient(env, obj, cl);

    return JNI_TRUE;
}

JNIEXPORT void JNICALL Java_com_coboltforge_dontmind_multivnc_VNCConn_rfbShutdown(JNIEnv *env, jobject obj) {
    rfbClient *cl = getRfbClient(env, obj);
    if(cl) {
        log_obj_tostring(env, obj,  "rfbShutdown() closing connection");
        close(cl->sock);

        if(cl->frameBuffer) {
            free(cl->frameBuffer);
            cl->frameBuffer = 0;
        }

        rfbClientCleanup(cl);
        // rfbClientCleanup does not zero the pointer
        setRfbClient(env, obj, 0);
    }
}

JNIEXPORT jboolean JNICALL Java_com_coboltforge_dontmind_multivnc_VNCConn_rfbInit(JNIEnv *env, jobject obj, jstring host, jint port) {
    log_obj_tostring(env, obj, "rfbInit()");

    if(!getRfbClient(env, obj))
        setupClient(env, obj);

    rfbClient *cl = getRfbClient(env, obj);

    cl->programName = "VNCConn";

    const char *cHost = (*env)->GetStringUTFChars(env, host, NULL);
    cl->serverHost = strdup(cHost);
    (*env)->ReleaseStringUTFChars(env, host, cHost);

    cl->serverPort = port;
    // Support short-form (:0, :1)
    if(cl->serverPort < 100)
        cl->serverPort += 5900;

    log_obj_tostring(env, obj, "rfbInit() about to connect to '%s', port %d\n", cl->serverHost, cl->serverPort);

    if(!rfbInitClient(cl, 0, NULL)) {
        setRfbClient(env, obj, 0); //  rfbInitClient() calls rfbClientCleanup() on failure, but this does not zero the ptr
        log_obj_tostring(env, obj, "rfbInit() failed. Cleanup by library.");
        return JNI_FALSE;
    }

    // if there was an error in alloc_framebuffer(), catch that here
    if(!cl->frameBuffer) {
        Java_com_coboltforge_dontmind_multivnc_VNCConn_rfbShutdown(env, obj);
        return JNI_FALSE;
    }

    return JNI_TRUE;
}


JNIEXPORT jboolean JNICALL Java_com_coboltforge_dontmind_multivnc_VNCConn_rfbProcessServerMessage(JNIEnv *env, jobject obj) {
    rfbClient *cl = getRfbClient(env, obj);

    /*
     * Save pointers to the managed VNCConn and env in the rfbClient for use in the onXYZ callbacks.
     * We do this each time here and not in setupClient() because the managed objects get moved
     * around by the VM.
     */
    rfbClientSetClientData(cl, VNCCONN_OBJ_ID, obj);
    rfbClientSetClientData(cl, VNCCONN_ENV_ID, env);

    /* request update and handle response */
    if(!rfbProcessServerMessage(cl, 500)) {
        if(errno == EINTR)
            return JNI_TRUE;

        log_obj_tostring(env, obj,"rfbProcessServerMessage() failed");
        return JNI_FALSE;
    }
    return JNI_TRUE;
}


JNIEXPORT jstring JNICALL Java_com_coboltforge_dontmind_multivnc_VNCConn_rfbGetDesktopName(JNIEnv *env, jobject obj) {
    rfbClient *cl = getRfbClient(env, obj);
    if(cl)
        return (*env)->NewStringUTF(env, cl->desktopName);
    else
        return NULL;
}

JNIEXPORT jboolean JNICALL Java_com_coboltforge_dontmind_multivnc_VNCConn_rfbSendKeyEvent(JNIEnv *env, jobject obj, jlong keysym, jboolean down) {
    rfbClient *cl = getRfbClient(env, obj);
    if(cl)
        return (jboolean) SendKeyEvent(cl, (uint32_t) keysym, down);
    else
        return JNI_FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//   rfbClient wrapper
//
//   1. It provides implementation for native methods of 'NativeRfbClient'.
//   2. It executes the managed callbacks when native callbacks are invoked.
///////////////////////////////////////////////////////////////////////////////

/******************************************************************************
  Some utilities for working with JNI
******************************************************************************/

/**
 * Returns a native copy of the given jstring.
 * Caller is responsible for releasing the memory.
 */
char *getNativeStrCopy(JNIEnv *env, jstring jStr) {
    const char *cStr = (*env)->GetStringUTFChars(env, jStr, NULL);
    char *str = strdup(cStr);
    (*env)->ReleaseStringUTFChars(env, jStr, cStr);
    return str;
}

/******************************************************************************
  Callbacks
  TODO: See if we can reduce the boilerplate required for calling managed methods
******************************************************************************/

static char *onGetPassword(rfbClient *client) {
    jobject obj = rfbClientGetClientData(client, JAVA_RFBCLIENT_OBJ_ID);
    JNIEnv *env = rfbClientGetClientData(client, JAVA_RFBCLIENT_ENV_ID);

    jclass cls = (*env)->GetObjectClass(env, obj);
    jmethodID mid = (*env)->GetMethodID(env, cls, "cbGetPassword", "()Ljava/lang/String;");
    jstring jPassword = (*env)->CallObjectMethod(env, obj, mid);

    return getNativeStrCopy(env, jPassword);
}

static rfbCredential *onGetCredential(rfbClient *client, int credentialType) {
    if (credentialType != rfbCredentialTypeUser) {
        //Only user credentials (i.e. username & password) are currently supported
        rfbClientErr("Unsupported credential type requested");
        return NULL;
    }

    jobject obj = rfbClientGetClientData(client, JAVA_RFBCLIENT_OBJ_ID);
    JNIEnv *env = rfbClientGetClientData(client, JAVA_RFBCLIENT_ENV_ID);

    //Retrieve credentials
    jclass cls = (*env)->GetObjectClass(env, obj);
    jmethodID mid = (*env)->GetMethodID(env, cls, "cbGetCredential", "()Lcom/coboltforge/dontmind/multivnc/NativeRfbClient/RfbUserCredential;");
    jobject jCredential = (*env)->CallObjectMethod(env, obj, mid);

    //Extract username & password
    jclass jCredentialCls = (*env)->GetObjectClass(env, jCredential);
    jfieldID usernameField = (*env)->GetFieldID(env, jCredentialCls, "username", "Ljava/lang/String;");
    jstring jUsername = (*env)->GetObjectField(env, jCredential, usernameField);

    jfieldID passwordField = (*env)->GetFieldID(env, jCredentialCls, "password", "Ljava/lang/String;");
    jstring jPassword = (*env)->GetObjectField(env, jCredential, passwordField);

    //Create native rfbCredential
    rfbCredential *credential = malloc(sizeof(rfbCredential));
    credential->userCredential.username = getNativeStrCopy(env, jUsername);
    credential->userCredential.password = getNativeStrCopy(env, jPassword);

    return credential;
}

static void onBell(rfbClient *client) {
    jobject obj = rfbClientGetClientData(client, JAVA_RFBCLIENT_OBJ_ID);
    JNIEnv *env = rfbClientGetClientData(client, JAVA_RFBCLIENT_ENV_ID);

    jclass cls = (*env)->GetObjectClass(env, obj);
    jmethodID mid = (*env)->GetMethodID(env, cls, "cbBell", "()V");
    (*env)->CallVoidMethod(env, obj, mid);
}

static void onGotXCutText(rfbClient *client, const char *text, int len) {
    jobject obj = rfbClientGetClientData(client, JAVA_RFBCLIENT_OBJ_ID);
    JNIEnv *env = rfbClientGetClientData(client, JAVA_RFBCLIENT_ENV_ID);

    jclass cls = (*env)->GetObjectClass(env, obj);
    jmethodID mid = (*env)->GetMethodID(env, cls, "cbGotXCutText", "(Ljava/lang/String;)V");
    jstring jText = (*env)->NewStringUTF(env, text);
    (*env)->CallVoidMethod(env, obj, mid, jText);
    (*env)->DeleteLocalRef(env, jText);
}

static rfbBool onHandleCursorPos(rfbClient *client, int x, int y) {
    jobject obj = rfbClientGetClientData(client, JAVA_RFBCLIENT_OBJ_ID);
    JNIEnv *env = rfbClientGetClientData(client, JAVA_RFBCLIENT_ENV_ID);

    jclass cls = (*env)->GetObjectClass(env, obj);
    jmethodID mid = (*env)->GetMethodID(env, cls, "cbHandleCursorPos", "(II)Z");
    jboolean result = (*env)->CallBooleanMethod(env, obj, mid, x, y);

    return result == JNI_TRUE ? TRUE : FALSE;
}

static void onFinishedFrameBufferUpdate(rfbClient *client) {
    jobject obj = rfbClientGetClientData(client, JAVA_RFBCLIENT_OBJ_ID);
    JNIEnv *env = rfbClientGetClientData(client, JAVA_RFBCLIENT_ENV_ID);

    jclass cls = (*env)->GetObjectClass(env, obj);
    jmethodID mid = (*env)->GetMethodID(env, cls, "cbFinishedFrameBufferUpdate", "()V");
    (*env)->CallVoidMethod(env, obj, mid);
}

/**
 * Hooks callbacks to rfbClient.
 */
static void setCallbacks(rfbClient *client) {
    client->GetPassword = onGetPassword;
    client->GetCredential = onGetCredential;
    client->Bell = onBell;
    client->GotXCutText = onGotXCutText;
    client->HandleCursorPos = onHandleCursorPos;
    client->FinishedFrameBufferUpdate = onFinishedFrameBufferUpdate;
}


/******************************************************************************
  Native method Implementation
******************************************************************************/

JNIEXPORT jlong JNICALL
Java_com_coboltforge_dontmind_multivnc_NativeRfbClient_nativeCreateClient(JNIEnv *env, jobject thiz) {
    rfbClient *client = rfbGetClient(BITSPERSAMPLE, SAMPLESPERPIXEL, BYTESPERPIXEL);

    setCallbacks(client);

    //TODO: Fixme: Allowing only few auths during testing because TLS is causing a segfault.
    //             It might be a bug in libvncclient OR I am doing something wrong.
    uint32_t auths[] = {rfbVncAuth, rfbMSLogon};
    SetClientAuthSchemes(client, auths, 2);

    return (jlong) client;
}

JNIEXPORT jboolean JNICALL
Java_com_coboltforge_dontmind_multivnc_NativeRfbClient_nativeInit(JNIEnv *env,
                                                                  jobject thiz,
                                                                  jlong clientPtr,
                                                                  jstring host,
                                                                  jint port) {
    rfbClient *client = (rfbClient *) clientPtr;
    rfbClientSetClientData(client, JAVA_RFBCLIENT_OBJ_ID, thiz);
    rfbClientSetClientData(client, JAVA_RFBCLIENT_ENV_ID, env);

    client->serverHost = getNativeStrCopy(env, host);
    client->serverPort = port < 100 ? port + 5900 : port;  // Support short-form (:0, :1)

    if (!rfbInitClient(client, 0, NULL)) {
        rfbClientErr("rfbInitClient() failed inside nativeInit().");
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_coboltforge_dontmind_multivnc_NativeRfbClient_nativeCleanup(JNIEnv *env,
                                                                     jobject thiz,
                                                                     jlong clientPtr) {
    rfbClient *client = (rfbClient *) clientPtr;

    if (client->frameBuffer) {
        free(client->frameBuffer);
        client->frameBuffer = 0;
    }

    rfbClientCleanup(client);
}

JNIEXPORT jboolean JNICALL
Java_com_coboltforge_dontmind_multivnc_NativeRfbClient_nativeProcessServerMessage(JNIEnv *env,
                                                                                  jobject thiz,
                                                                                  jlong clientPtr,
                                                                                  jint usecTimeout) {
    rfbClient *client = (rfbClient *) clientPtr;

    /*
     * Save pointers to the managed RfbClient and env in the rfbClient for use in the onXYZ callbacks.
     * We do this each time here and not in createClient() because the managed objects get moved
     * around by the VM.
     */
    rfbClientSetClientData(client, JAVA_RFBCLIENT_OBJ_ID, thiz);
    rfbClientSetClientData(client, JAVA_RFBCLIENT_ENV_ID, env);

    if (!rfbProcessServerMessage(client, usecTimeout)) {
        if (errno != EINTR) {
            log_obj_tostring(env, thiz, "nativeProcessServerMessage() failed");
            return JNI_FALSE;
        }
    }
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_coboltforge_dontmind_multivnc_NativeRfbClient_nativeSendKeyEvent(JNIEnv *env,
                                                                          jobject thiz,
                                                                          jlong clientPtr,
                                                                          jlong key,
                                                                          jboolean isDown) {
    return (jboolean) SendKeyEvent((rfbClient *) clientPtr, (uint32_t) key, isDown);
}

JNIEXPORT jboolean JNICALL
Java_com_coboltforge_dontmind_multivnc_NativeRfbClient_nativeSendPointerEvent(JNIEnv *env,
                                                                              jobject thiz,
                                                                              jlong clientPtr,
                                                                              jint x, jint y,
                                                                              jint mask) {
    return (jboolean) SendPointerEvent((rfbClient *) clientPtr, x, y, mask);
}

JNIEXPORT jboolean JNICALL
Java_com_coboltforge_dontmind_multivnc_NativeRfbClient_nativeSendCutText(JNIEnv *env,
                                                                         jobject thiz,
                                                                         jlong clientPtr,
                                                                         jstring text) {
    char *cText = getNativeStrCopy(env, text);
    rfbBool result = SendClientCutText((rfbClient *) clientPtr, cText, strlen(cText));
    free(cText);
    return (jboolean) result;
}

JNIEXPORT jboolean JNICALL
Java_com_coboltforge_dontmind_multivnc_NativeRfbClient_nativeSendFrameBufferUpdateRequest(JNIEnv *env,
                                                                                          jobject thiz,
                                                                                          jlong clientPtr,
                                                                                          jint x,
                                                                                          jint y,
                                                                                          jint w,
                                                                                          jint h,
                                                                                          jboolean incremental) {
    return (jboolean) SendFramebufferUpdateRequest((rfbClient *) clientPtr, x, y, w, h, incremental);
}

JNIEXPORT jobject JNICALL
Java_com_coboltforge_dontmind_multivnc_NativeRfbClient_nativeGetConnectionInfo(JNIEnv *env,
                                                                               jobject thiz,
                                                                               jlong clientPtr) {
    rfbClient *client = (rfbClient *) clientPtr;

    jclass infoClass = (*env)->FindClass(env, "com/coboltforge/dontmind/multivnc/NativeRfbClient$ConnectionInfo");
    jmethodID ctorId = (*env)->GetMethodID(env, infoClass, "<init>", "(Ljava/lang/String;IIZ)V");
    if (ctorId == NULL) {
        rfbClientErr("Could not find the constructor for 'ConnectionInfo'. Constructor signature may be incorrect");
    }

    // Important: Keep the arguments in sync with 'ConnectionInfo' constructor.
    jobject infoObject = (*env)->NewObject(env, infoClass, ctorId,
                                           (*env)->NewStringUTF(env, client->desktopName),  // desktopName
                                           client->width,                                   // frameWidth
                                           client->height,                                  // frameHeight
                                           client->tlsSession ? JNI_TRUE : JNI_FALSE);      // isEncrypted

    return infoObject;
}
