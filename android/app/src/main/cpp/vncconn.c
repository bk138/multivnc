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
#include <netdb.h>
#include <errno.h>
#include <libsshtunnel.h>
#include <rfb/rfbclient.h>

#define TAG "VNCConn-native"

/* id for the managed VNCConn */
#define VNCCONN_OBJ_ID (void*)1
#define VNCCONN_ENV_ID (void*)2
#define VNCCONN_SSH_ID (void*)3


/*
 * Modeled after rfbDefaultClientLog:
 *  - with Android log functions
 *  - without time stamping as the Android logging does this already
 * There's no per-connection log since we cannot find out which client
 * called the logger function :-(
 */
static void logcat_info(const char *format, ...)
{
    va_list args;

    if(!rfbEnableClientLogging)
        return;

    va_start(args, format);
    __android_log_vprint(ANDROID_LOG_INFO, TAG, format, args);
    va_end(args);
}

static void logcat_err(const char *format, ...)
{
    va_list args;

    if(!rfbEnableClientLogging)
        return;

    va_start(args, format);
    __android_log_vprint(ANDROID_LOG_ERROR, TAG, format, args);
    va_end(args);
}

static void log_obj_tostring(JNIEnv *env, jobject obj, int prio, const char *format, ...) {

    if(!env)
        return;

    jclass cls = (*env)->GetObjectClass(env, obj);
    jmethodID mid = (*env)->GetMethodID(env, cls, "toString", "()Ljava/lang/String;");
    jstring jStr = (*env)->CallObjectMethod(env, obj, mid);
    const char *cStr = (*env)->GetStringUTFChars(env, jStr, NULL);
    if(!cStr)
        return;

    va_list args;

    /* prefix format string with result of toString() */
    const size_t format_buf_len = 1024;
    char *format_buf[format_buf_len];
    snprintf((char*)format_buf, format_buf_len, "%s: %s", cStr, format);

    va_start(args, format);
    __android_log_vprint(prio, TAG, (char*)format_buf, args);
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

    rfbClientLog = logcat_info;
    rfbClientErr = logcat_err;

    // this is not thread-safe so run here once
    if(ssh_tunnel_init()) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "libsshtunnel initialization failed\n");
    }

    return JNI_VERSION_1_6;
}


/**
 * Get the managed VNCConn's rfbClient.
 */
static rfbClient* getRfbClient(JNIEnv *env, jobject conn) {

    if(!env) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "getRfbClient failed due to env NULL");
        return NULL;
    }

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

    if(!env) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "setRfbClient failed due to env NULL");
        return JNI_FALSE;
    }

    jclass cls = (*env)->GetObjectClass(env, conn);
    jfieldID fid = (*env)->GetFieldID(env, cls, "rfbClient", "J");
    if (fid == 0)
        return JNI_FALSE;

    (*env)->SetLongField(env, conn, fid, (long)cl);

    return JNI_TRUE;
}


static void onFramebufferUpdateFinished(rfbClient* client)
{
    if(!client) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "onFramebufferUpdateFinished failed due to client NULL");
        return;
    }

    jobject obj = rfbClientGetClientData(client, VNCCONN_OBJ_ID);
    JNIEnv *env = rfbClientGetClientData(client, VNCCONN_ENV_ID);

    if(!env || !obj) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "onFramebufferUpdateFinished failed due to env or obj NULL");
        return;
    }

    jclass cls = (*env)->GetObjectClass(env, obj);
    jmethodID mid = (*env)->GetMethodID(env, cls, "onFramebufferUpdateFinished", "()V");
    (*env)->CallVoidMethod(env, obj, mid);
}

static void onGotCutText(rfbClient *client, const char *text, int len)
{
    if(!client) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "onGotCutText failed due to client NULL");
        return;
    }

    jobject obj = rfbClientGetClientData(client, VNCCONN_OBJ_ID);
    JNIEnv *env = rfbClientGetClientData(client, VNCCONN_ENV_ID);

    if(!env || !obj) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "onGotCutText failed due to env or obj NULL");
        return;
    }

    jclass cls = (*env)->GetObjectClass(env, obj);
    jmethodID mid = (*env)->GetMethodID(env, cls, "onGotCutText", "([B)V");
    jbyteArray jBytes = (*env)->NewByteArray(env, len);
    (*env)->SetByteArrayRegion(env, jBytes, 0, len, (jbyte *) text);
    (*env)->CallVoidMethod(env, obj, mid, jBytes);
}

static char *onGetPassword(rfbClient *client)
{
    if(!client) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "onGetPassword failed due to client NULL");
        return NULL;
    }

    jobject obj = rfbClientGetClientData(client, VNCCONN_OBJ_ID);
    JNIEnv *env = rfbClientGetClientData(client, VNCCONN_ENV_ID);

    if(!env || !obj) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "onGetPassword failed due to env or obj NULL");
        return NULL;
    }

    jclass cls = (*env)->GetObjectClass(env, obj);
    jmethodID mid = (*env)->GetMethodID(env, cls, "onGetPassword", "()Ljava/lang/String;");
    jstring passwd = (*env)->CallObjectMethod(env, obj, mid);

    const char *cPasswd = (*env)->GetStringUTFChars(env, passwd, NULL);
    if(!cPasswd) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "onGetPassword failed due to cPasswd NULL");
        return NULL;
    }
    char *cPasswdCopy = strdup(cPasswd); // this is free()'d by LibVNCClient in HandleVncAuth()
    (*env)->ReleaseStringUTFChars(env, passwd, cPasswd);

    return cPasswdCopy;
}

static rfbCredential *onGetCredential(rfbClient *client, int credentialType)
{
    if(!client) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "onGetCredential failed due to client NULL");
        return NULL;
    }

    if (credentialType != rfbCredentialTypeUser) {
        //Only user credentials (i.e. username & password) are currently supported
        __android_log_print(ANDROID_LOG_ERROR, TAG, "onGetCredential failed due to unsupported credential type %d requested", credentialType);
        return NULL;
    }

    jobject obj = rfbClientGetClientData(client, VNCCONN_OBJ_ID);
    JNIEnv *env = rfbClientGetClientData(client, VNCCONN_ENV_ID);

    if(!env || !obj) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "onGetCredential failed due to env or obj NULL");
        return NULL;
    }

    // Retrieve credentials
    jclass cls = (*env)->GetObjectClass(env, obj);
    jmethodID mid = (*env)->GetMethodID(env, cls, "onGetUserCredential", "()Lcom/coboltforge/dontmind/multivnc/VNCConn$UserCredential;");
    jobject jCredential = (*env)->CallObjectMethod(env, obj, mid);

    // Extract username & password
    jclass jCredentialCls = (*env)->GetObjectClass(env, jCredential);
    jfieldID usernameField = (*env)->GetFieldID(env, jCredentialCls, "username", "Ljava/lang/String;");
    jstring jUsername = (*env)->GetObjectField(env, jCredential, usernameField);

    jfieldID passwordField = (*env)->GetFieldID(env, jCredentialCls, "password", "Ljava/lang/String;");
    jstring jPassword = (*env)->GetObjectField(env, jCredential, passwordField);

    // Create native rfbCredential, this is free()'d by FreeUserCredential() in LibVNCClient
    rfbCredential *credential = malloc(sizeof(rfbCredential));
    if(!credential) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "onGetCredential failed due to credential NULL");
        return NULL;
    }

    const char *cUsername = (*env)->GetStringUTFChars(env, jUsername, NULL);
    if(!cUsername) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "onGetCredential failed due to cUsername NULL");
        return NULL;
    }
    credential->userCredential.username = strdup(cUsername);
    (*env)->ReleaseStringUTFChars(env, jUsername, cUsername);

    const char *cPassword = (*env)->GetStringUTFChars(env, jPassword, NULL);
    if(!cPassword) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "onGetCredential failed due to cPassword NULL");
        return NULL;
    }
    credential->userCredential.password = strdup(cPassword);
    (*env)->ReleaseStringUTFChars(env, jPassword, cPassword);

    return credential;
}

MallocFrameBufferProc defaultMallocFramebuffer;
/**
 * This basically is just a thin wrapper around the libraries built-in framebuffer resizing that
 * conveys width/height changes up to the managed VNCConn.
 * @param client
 * @return
 */
static rfbBool onNewFBSize(rfbClient *client)
{
    if(!client) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "onNewFBSize failed due to client NULL");
        return FALSE;
    }

    jobject obj = rfbClientGetClientData(client, VNCCONN_OBJ_ID);
    JNIEnv *env = rfbClientGetClientData(client, VNCCONN_ENV_ID);

    if(!env || !obj) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "onNewFBSize failed due to env or obj NULL");
        return FALSE;
    }

    jclass cls = (*env)->GetObjectClass(env, obj);
    jmethodID mid = (*env)->GetMethodID(env, cls, "onNewFramebufferSize", "(II)V");
    (*env)->CallVoidMethod(env, obj, mid, client->width, client->height);

    return defaultMallocFramebuffer(client);
}

static void onSshError(void *client, ssh_tunnel_error_t error_code,  const char *error_message) {
    __android_log_print(ANDROID_LOG_ERROR, TAG,"libsshtunnel error: %d - %s", error_code, error_message);
}

/**
   Decide whether or not the SSH tunnel setup should continue
   based on the current host and its fingerprint.
   Business logic is up to the implementer in a real app, i.e.
   compare keys, ask user etc...
   @return -1 if tunnel setup should be aborted
            0 if tunnel setup should continue
 */
static int onSshFingerprintCheck(void *client,
                                 const char *fingerprint,
                                 int fingerprint_len,
                                 const char *host)
{
    if(!client) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "onSshFingerprintCheck failed due to client NULL");
        return -1;
    }

    jobject obj = rfbClientGetClientData(client, VNCCONN_OBJ_ID);
    JNIEnv *env = rfbClientGetClientData(client, VNCCONN_ENV_ID);

    if(!env || !obj) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "onSshFingerprintCheck failed due to env or obj NULL");
        return -1;
    }

    jbyteArray jFingerprint = (*env)->NewByteArray(env, (jsize)fingerprint_len);
    (*env)->SetByteArrayRegion(env, jFingerprint, 0, (jsize)fingerprint_len, (jbyte *) fingerprint);

    jclass cls = (*env)->GetObjectClass(env, obj);
    jmethodID mid = (*env)->GetMethodID(env, cls, "onSshFingerprintCheck", "(Ljava/lang/String;[B)I");
    return  (*env)->CallIntMethod(env, obj, mid, (*env)->NewStringUTF(env, host), jFingerprint);
}


/**
 * Allocates and sets up the VNCConn's rfbClient.
 * @param env
 * @param obj
 * @param bytesPerPixel
 * @return
 */
static jboolean setupClient(JNIEnv *env, jobject obj, jint bytesPerPixel) {

    log_obj_tostring(env, obj, ANDROID_LOG_INFO, "setupClient()");

    if(getRfbClient(env, obj)) { /* already set up */
        log_obj_tostring(env, obj, ANDROID_LOG_INFO, "setupClient() already done");
        return JNI_FALSE;
    }

    rfbClient *cl = NULL;

    /*
     * We only allow 24 and 15 bit colour, as the GL canvas is not able to digest any other format
     * _directly_, without converting the whole client framebuffer byte-per-byte.
    */
    switch(bytesPerPixel) {
        case 2:
            // 15-bit colour
            cl = rfbGetClient(5, 3, 2);
            // the GL canvas is using GL_UNSIGNED_SHORT_5_5_5_1 for 15-bit colour depth
            cl->format.redShift = 11;
            cl->format.greenShift = 6;
            cl->format.blueShift = 1;
            break;
        case 4:
            // 24-bit colour, occupying 4 bytes
            cl = rfbGetClient(8, 3, 4);
            break;
        default:
            break;
    }

    if(!cl) {
        log_obj_tostring(env, obj, ANDROID_LOG_ERROR, "setupClient() failed due to client NULL");
        return JNI_FALSE;
    }

    // set callbacks
    cl->FinishedFrameBufferUpdate = onFramebufferUpdateFinished;
    cl->GotXCutText = onGotCutText;
    cl->GetPassword = onGetPassword;
    cl->GetCredential = onGetCredential;
    defaultMallocFramebuffer = cl->MallocFrameBuffer; // save default one
    cl->MallocFrameBuffer = onNewFBSize; // set new one

    // set flags
    cl->canHandleNewFBSize = TRUE;

    /*
     * Save pointers to the managed VNCConn and env in the rfbClient for use in the onXYZ callbacks.
     * In addition to rfbProcessServerMessage(), we have to do this are as some callbacks (namely
     * related to authentication) are called before rfbProcessServerMessage().
    */
    rfbClientSetClientData(cl, VNCCONN_OBJ_ID, obj);
    rfbClientSetClientData(cl, VNCCONN_ENV_ID, env);

    setRfbClient(env, obj, cl);

    return JNI_TRUE;
}

JNIEXPORT void JNICALL Java_com_coboltforge_dontmind_multivnc_VNCConn_rfbShutdown(JNIEnv *env, jobject obj) {
    rfbClient *cl = getRfbClient(env, obj);
    if(cl) {
        log_obj_tostring(env, obj, ANDROID_LOG_INFO, "rfbShutdown() closing connection");
        rfbCloseSocket(cl->sock);

        ssh_tunnel_close(rfbClientGetClientData(cl, VNCCONN_SSH_ID));

        if(cl->frameBuffer) {
            free(cl->frameBuffer);
            cl->frameBuffer = 0;
        }

        rfbClientCleanup(cl);
        // rfbClientCleanup does not zero the pointer
        setRfbClient(env, obj, 0);
    }
}


JNIEXPORT jboolean JNICALL Java_com_coboltforge_dontmind_multivnc_VNCConn_rfbInit(JNIEnv *env, jobject obj, jstring host, jint port, jint repeaterId, jint bytesPerPixel,
                                                                                  jstring encodingsString,
                                                                                  jboolean enableCompress,
                                                                                  jboolean enableJPEG,
                                                                                  jint compressLevel,
                                                                                  jint qualityLevel,
                                                                                  jstring ssh_host,
                                                                                  jstring ssh_user,
                                                                                  jstring ssh_password,
                                                                                  jbyteArray ssh_priv_key,
                                                                                  jstring ssh_priv_key_password) {

    if(!getRfbClient(env, obj))
        setupClient(env, obj, bytesPerPixel);

    rfbClient *cl = getRfbClient(env, obj);

    if(!cl) {
        log_obj_tostring(env, obj, ANDROID_LOG_ERROR, "rfbInit() failed due to client NULL");
        return JNI_FALSE;
    }

    cl->programName = "VNCConn";

    /*
     * Get all C representations from managec code. The checks for NULL are needed
     * as the Get*() methods fail on a null reference.
     */
    const char *cHost = host ? (*env)->GetStringUTFChars(env, host, NULL) : NULL;
    const char *cEncodingsString = encodingsString ? (*env)->GetStringUTFChars(env, encodingsString, NULL) : NULL;
    const char *cSshHost = ssh_host ? (*env)->GetStringUTFChars(env, ssh_host, NULL) : NULL;
    const char *cSshUser = ssh_user ? (*env)->GetStringUTFChars(env, ssh_user, NULL) : NULL;
    const char *cSshPassword = ssh_password ? (*env)->GetStringUTFChars(env, ssh_password, NULL) : NULL;
    jbyte *cSshPrivKey = ssh_priv_key ? (*env)->GetByteArrayElements(env, ssh_priv_key, NULL) : NULL;
    jsize cSshPrivKeyLen = ssh_priv_key ? (*env)->GetArrayLength(env, ssh_priv_key) : 0;
    const char *cSshPrivKeyPassword = ssh_priv_key_password ? (*env)->GetStringUTFChars(env, ssh_priv_key_password, NULL) : NULL;

    // see whether we are ssh-tunneling or not
    int is_ssh_connection = cSshHost != NULL;

    if(is_ssh_connection) {
        log_obj_tostring(env, obj, ANDROID_LOG_INFO, "rfbInit() setting up SSH-tunneled connection");
        // ssh-tunneling, check whether it's password- or key-based
        ssh_tunnel_t *tunnel;
        if(cSshPassword) {
            // password-based
            tunnel = ssh_tunnel_open_with_password(cSshHost, cSshUser, cSshPassword, cHost, port, cl, onSshFingerprintCheck, onSshError);
        } else {
            // key-based
            tunnel = ssh_tunnel_open_with_privkey(cSshHost, cSshUser, (char*)cSshPrivKey, cSshPrivKeyLen, cSshPrivKeyPassword, cHost, port, cl, onSshFingerprintCheck, onSshError);
        }

        cl->serverHost = strdup("127.0.0.1");
        if(tunnel) // might be NULL if ssh setup failed
            cl->serverPort = ssh_tunnel_get_port(tunnel);
        rfbClientSetClientData(cl, VNCCONN_SSH_ID, tunnel);
    } else {
        log_obj_tostring(env, obj, ANDROID_LOG_INFO, "rfbInit() setting up direct connection");
        // direct connection
        if(cHost) // strdup(NULL) is UB
            cl->serverHost = strdup(cHost);

        cl->serverPort = port;
        // Support short-form (:0, :1)
        if(cl->serverPort < 100)
            cl->serverPort += 5900;
    }
    
    if(cEncodingsString) // strdup(NULL) is UB
        cl->appData.encodingsString = strdup(cEncodingsString);
        
    cl->appData.compressLevel = compressLevel;

    cl->appData.enableJPEG = enableJPEG;
    cl->appData.qualityLevel = qualityLevel;        


    // release all handles to managed strings again
    if (cHost)
        (*env)->ReleaseStringUTFChars(env, host, cHost);
    if(cEncodingsString)
        (*env)->ReleaseStringUTFChars(env, encodingsString, cEncodingsString);
    if (cSshHost)
        (*env)->ReleaseStringUTFChars(env, ssh_host, cSshHost);
    if(cSshUser)
        (*env)->ReleaseStringUTFChars(env, ssh_user, cSshUser);
    if(cSshPassword)
        (*env)->ReleaseStringUTFChars(env, ssh_password, cSshPassword);
    if (cSshPrivKey)
        (*env)->ReleaseByteArrayElements(env, ssh_priv_key, cSshPrivKey, JNI_ABORT);
    if (cSshPrivKeyPassword)
        (*env)->ReleaseStringUTFChars(env, ssh_priv_key_password, cSshPrivKeyPassword);

    // for both direct and ssh-tunneled connection, set repeater
    if(repeaterId >= 0) {
        cl->destHost = strdup("ID");
        cl->destPort = repeaterId;
    }


    // check if ssh connection was wanted and succeeded
    if(is_ssh_connection && !rfbClientGetClientData(cl, VNCCONN_SSH_ID)) {
        // ssh connection failed, bail out now
        log_obj_tostring(env, obj, ANDROID_LOG_ERROR, "rfbInit() SSH tunnel failed.");
        Java_com_coboltforge_dontmind_multivnc_VNCConn_rfbShutdown(env, obj);
        return JNI_FALSE;
    }

    log_obj_tostring(env, obj, ANDROID_LOG_INFO, "rfbInit() about to connect to '%s', port %d, repeaterId %d\n", cl->serverHost, cl->serverPort, cl->destPort);

    if(!rfbInitClient(cl, 0, NULL)) {
        setRfbClient(env, obj, 0); //  rfbInitClient() calls rfbClientCleanup() on failure, but this does not zero the ptr
        log_obj_tostring(env, obj, ANDROID_LOG_ERROR, "rfbInit() connection failed. Cleanup by library.");
        return JNI_FALSE;
    }

    // if there was an error in alloc_framebuffer(), catch that here
    if(!cl->frameBuffer) {
        log_obj_tostring(env, obj, ANDROID_LOG_ERROR, "rfbInit() failed due to framebuffer NULL");
        Java_com_coboltforge_dontmind_multivnc_VNCConn_rfbShutdown(env, obj);
        return JNI_FALSE;
    }

    return JNI_TRUE;
}


JNIEXPORT jboolean JNICALL Java_com_coboltforge_dontmind_multivnc_VNCConn_rfbProcessServerMessage(JNIEnv *env, jobject obj) {
    rfbClient *cl = getRfbClient(env, obj);

    if(!cl) {
        log_obj_tostring(env, obj, ANDROID_LOG_ERROR, "rfbProcessServerMessage() failed due to client NULL");
        return JNI_FALSE;
    }

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

        log_obj_tostring(env, obj, ANDROID_LOG_ERROR, "rfbProcessServerMessage() failed");
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
JNIEXPORT jboolean JNICALL Java_com_coboltforge_dontmind_multivnc_VNCConn_rfbSetFramebufferUpdatesEnabled(JNIEnv *env, jobject obj, jboolean enable) {
    rfbClient *cl = getRfbClient(env, obj);
    if(cl) {
        log_obj_tostring(env, obj, ANDROID_LOG_INFO, "rfbSetFramebufferUpdatesEnabled() %d", enable);
        if(enable) {
            // set to managed-by-lib again
            rfbClientSetUpdateRect(cl, NULL);
            // request full update
            SendFramebufferUpdateRequest(cl, 0, 0, cl->width, cl->height, FALSE);
        } else {
            // set to no-updates-wanted
            rfbRectangle noRect = {0,0,0,0,};
            rfbClientSetUpdateRect(cl, &noRect);
        }
        return JNI_TRUE;
    }
    else
        return JNI_FALSE;
}
#pragma clang diagnostic pop

JNIEXPORT jstring JNICALL Java_com_coboltforge_dontmind_multivnc_VNCConn_rfbGetDesktopName(JNIEnv *env, jobject obj) {
    rfbClient *cl = getRfbClient(env, obj);
    if(cl)
        return (*env)->NewStringUTF(env, cl->desktopName);
    else
        return NULL;
}

JNIEXPORT jint JNICALL Java_com_coboltforge_dontmind_multivnc_VNCConn_rfbGetFramebufferWidth(JNIEnv *env, jobject obj) {
    rfbClient *cl = getRfbClient(env, obj);
    if(cl)
        return cl->width;
    else
        return 0;
}

JNIEXPORT jint JNICALL Java_com_coboltforge_dontmind_multivnc_VNCConn_rfbGetFramebufferHeight(JNIEnv *env, jobject obj) {
    rfbClient *cl = getRfbClient(env, obj);
    if(cl)
        return cl->height;
    else
        return 0;
}

JNIEXPORT jboolean JNICALL Java_com_coboltforge_dontmind_multivnc_VNCConn_rfbSendKeyEvent(JNIEnv *env, jobject obj, jlong keysym, jboolean down) {
    rfbClient *cl = getRfbClient(env, obj);
    if(cl)
        return (jboolean) SendKeyEvent(cl, (uint32_t) keysym, down);
    else
        return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_com_coboltforge_dontmind_multivnc_VNCConn_rfbSendPointerEvent(JNIEnv *env, jobject obj, jint x, jint y, jint buttonMask) {
    rfbClient *cl = getRfbClient(env, obj);
    if(cl)
        return (jboolean) SendPointerEvent(cl, x, y, buttonMask);
    else
        return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_com_coboltforge_dontmind_multivnc_VNCConn_rfbSendClientCutText(JNIEnv *env, jobject obj, jbyteArray bytes) {
    rfbClient *cl = getRfbClient(env, obj);
    if (cl) {
        jbyte *cText = (*env)->GetByteArrayElements(env, bytes, NULL);
        int cTextLen = (*env)->GetArrayLength(env, bytes);
        jboolean status = SendClientCutText(cl, (char *) cText, cTextLen);
        (*env)->ReleaseByteArrayElements(env, bytes, cText, JNI_ABORT);
        return status;
    }
    else
        return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_com_coboltforge_dontmind_multivnc_VNCConn_rfbIsEncrypted(JNIEnv *env, jobject obj) {
    rfbClient *cl = getRfbClient(env, obj);
    if (cl)
        return cl->tlsSession != NULL || rfbClientGetClientData(cl, VNCCONN_SSH_ID) != NULL;
    else
        return JNI_FALSE;
}