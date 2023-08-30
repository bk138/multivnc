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
#include <libssh2.h>
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
 * SSH-related stuff
 */

static int onSshFingerprintCheck(const char *fingerprint, size_t fingerprint_len,
                                 const char *host, rfbClient *client);


typedef struct
{
    rfbClient *client;
    LIBSSH2_SESSION *session;
    pthread_t thread;
    int ssh_sock;
    int local_listensock;
    int local_listenport;
    char *remote_desthost;
    int remote_destport;
} SshData;


THREAD_ROUTINE_RETURN_TYPE ssh_proxy_loop(void *arg)
{
    SshData *data = arg;
    int rc, i;
    struct sockaddr_in sin;
    socklen_t sinlen;
    LIBSSH2_CHANNEL *channel = NULL;
    const char *shost;
    int sport;
    fd_set fds;
    struct timeval tv;
    ssize_t len, wr;
    char buf[16384];
    int proxy_sock = RFB_INVALID_SOCKET;

    proxy_sock = accept(data->local_listensock, (struct sockaddr *)&sin, &sinlen);
    if(proxy_sock == RFB_INVALID_SOCKET) {
         __android_log_print(ANDROID_LOG_ERROR, TAG, "ssh_proxy_loop: accept: %s\n", strerror(errno));
        goto shutdown;
    }

    /* Close listener once a connection got accepted */
    rfbCloseSocket(data->local_listensock);

    shost = inet_ntoa(sin.sin_addr);
    sport = ntohs(sin.sin_port);

     __android_log_print(ANDROID_LOG_INFO, TAG,"ssh_proxy_loop: forwarding connection from %s:%d here to remote %s:%d\n",
           shost, sport, data->remote_desthost, data->remote_destport);

    channel = libssh2_channel_direct_tcpip_ex(data->session, data->remote_desthost,
                                              data->remote_destport, shost, sport);
    if(!channel) {
         __android_log_print(ANDROID_LOG_ERROR, TAG, "ssh_proxy_loop: Could not open the direct-tcpip channel!\n"
                        "(Note that this can be a problem at the server!"
                        " Please review the server logs.)\n");
        goto shutdown;
    }

    /* Must use non-blocking IO hereafter due to the current libssh2 API */
    libssh2_session_set_blocking(data->session, 0);

    while(1) {
        FD_ZERO(&fds);
        FD_SET(proxy_sock, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        rc = select(proxy_sock + 1, &fds, NULL, NULL, &tv);
        if(-1 == rc) {
             __android_log_print(ANDROID_LOG_ERROR, TAG, "ssh_proxy_loop: select: %s\n", strerror(errno));
            goto shutdown;
        }
        if(rc && FD_ISSET(proxy_sock, &fds)) {
            len = recv(proxy_sock, buf, sizeof(buf), 0);
            if(len < 0) {
                 __android_log_print(ANDROID_LOG_ERROR, TAG, "read: %s\n", strerror(errno));
                goto shutdown;
            }
            else if(0 == len) {
                 __android_log_print(ANDROID_LOG_ERROR, TAG, "ssh_proxy_loop: the client at %s:%d disconnected!\n", shost,
                        sport);
                goto shutdown;
            }
            wr = 0;
            while(wr < len) {
                i = libssh2_channel_write(channel, buf + wr, len - wr);
                if(LIBSSH2_ERROR_EAGAIN == i) {
                    continue;
                }
                if(i < 0) {
                     __android_log_print(ANDROID_LOG_ERROR, TAG, "ssh_proxy_loop: libssh2_channel_write: %d\n", i);
                    goto shutdown;
                }
                wr += i;
            }
        }
        while(1) {
            len = libssh2_channel_read(channel, buf, sizeof(buf));
            if(LIBSSH2_ERROR_EAGAIN == len)
                break;
            else if(len < 0) {
                 __android_log_print(ANDROID_LOG_ERROR, TAG, "ssh_proxy_loop: libssh2_channel_read: %d\n", (int)len);
                goto shutdown;
            }
            wr = 0;
            while(wr < len) {
                i = send(proxy_sock, buf + wr, len - wr, 0);
                if(i <= 0) {
                     __android_log_print(ANDROID_LOG_ERROR, TAG, "ssh_proxy_loop: write: %s\n", strerror(errno));
                    goto shutdown;
                }
                wr += i;
            }
            if(libssh2_channel_eof(channel)) {
                 __android_log_print(ANDROID_LOG_ERROR, TAG, "ssh_proxy_loop: the server at %s:%d disconnected!\n",
                        data->remote_desthost, data->remote_destport);
                goto shutdown;
            }
        }
    }

    shutdown:

     __android_log_print(ANDROID_LOG_INFO, TAG,"ssh_proxy_loop: shutting down\n");

    rfbCloseSocket(proxy_sock);

    if(channel)
        libssh2_channel_free(channel);

    libssh2_session_disconnect(data->session, "Client disconnecting normally");
    libssh2_session_free(data->session);

    rfbCloseSocket(data->ssh_sock);

    return THREAD_ROUTINE_RETURN_VALUE;
}


/**
   Creates an SSH tunnel and a local proxy and returns the port the proxy is listening on.
   @return A pointer to an SshData structure or NULL on error.
 */
SshData* ssh_tunnel_open(const char *ssh_host,
                         const char *ssh_user,
                         const char *ssh_password,
                         const signed char *ssh_priv_key,
                         int ssh_priv_key_len,
                         const char *ssh_priv_key_password,
                         const char *rfb_host,
                         int rfb_port,
                         rfbClient *client)
{
    int rc, i;
    struct sockaddr_in sin;
    socklen_t sinlen;
    const char *fingerprint;
    char *userauthlist;
    struct addrinfo hints, *res;
    SshData *data;

    /* Sanity checks */
    if(!ssh_host || !ssh_user || !rfb_host) /* these must be set */
        return NULL;

    data = calloc(1, sizeof(SshData));
    // set the sockets to invalid so we don't close 0 inadvertently
    data->local_listensock = data->ssh_sock = RFB_INVALID_SOCKET;

    data->client = client;
    data->remote_desthost = strdup(rfb_host); /* resolved by the server */
    data->remote_destport = rfb_port;

    /* Connect to SSH server */
    data->ssh_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(data->ssh_sock == RFB_INVALID_SOCKET) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "ssh_tunnel_open: socket: %s\n", strerror(errno));
        goto error;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((rc = getaddrinfo(ssh_host, NULL, &hints, &res)) == 0) {
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = (((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr);
        freeaddrinfo(res);
    } else {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "ssh_tunnel_open: getaddrinfo: %s\n", gai_strerror(rc));
        goto error;
    }

    sin.sin_port = htons(22);
    if(connect(data->ssh_sock, (struct sockaddr*)(&sin), sizeof(struct sockaddr_in)) != 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "ssh_tunnel_open: failed to connect to SSH server!\n");
        goto error;
    }

    /* Create a session instance */
    data->session = libssh2_session_init();
    if(!data->session) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "ssh_tunnel_open: could not initialize SSH session!\n");
        goto error;
    }

    /* ... start it up. This will trade welcome banners, exchange keys,
     * and setup crypto, compression, and MAC layers
     */
    rc = libssh2_session_handshake(data->session, data->ssh_sock);
    if(rc) {
        char *error_msg;
        libssh2_session_last_error(data->session, &error_msg, NULL, 0);
        __android_log_print(ANDROID_LOG_ERROR, TAG, "ssh_tunnel_open: error when starting up SSH session: %d: %s\n", rc, error_msg);
        goto error;
    }

    /* At this point we havn't yet authenticated.  The first thing to do
     * is check the hostkey's fingerprint against our known hosts Your app
     * may have it hard coded, may go to a file, may present it to the
     * user, that's your call
     */
    fingerprint = libssh2_hostkey_hash(data->session, LIBSSH2_HOSTKEY_HASH_SHA256);
    if(onSshFingerprintCheck(fingerprint, 32, ssh_host, data->client) == -1) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "ssh_tunnel_open: fingerprint check indicated tunnel setup stop\n");
        goto error;
    }

    /* check what authentication methods are available */
    userauthlist = libssh2_userauth_list(data->session, ssh_user, strlen(ssh_user));
    __android_log_print(ANDROID_LOG_INFO, TAG,"ssh_tunnel_open: authentication methods: %s\n", userauthlist);

    if(ssh_password && strstr(userauthlist, "password")) {
        if(libssh2_userauth_password(data->session, ssh_user, ssh_password)) {
            __android_log_print(ANDROID_LOG_ERROR, TAG, "ssh_tunnel_open: authentication by password failed.\n");
            goto error;
        }
    }
    else if(ssh_priv_key && ssh_priv_key_password && strstr(userauthlist, "publickey")) {
        if(libssh2_userauth_publickey_frommemory(data->session,
                                                 ssh_user, strlen(ssh_user),
                                                 NULL, 0,
                                                 (const char*)ssh_priv_key, ssh_priv_key_len,
                                                 ssh_priv_key_password)) {
            __android_log_print(ANDROID_LOG_ERROR, TAG, "ssh_tunnel_open: authentication by public key failed!\n");
            goto error;
        }
    }
    else {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "ssh_tunnel_open: no supported authentication methods found!\n");
        goto error;
    }

    /* Create and bind the local listening socket */
    data->local_listensock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(data->local_listensock == RFB_INVALID_SOCKET) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "ssh_tunnel_open: socket: %s\n", strerror(errno));
        return NULL;
    }
    sin.sin_family = AF_INET;
    sin.sin_port = htons(0); /* let the OS choose the port */
    sin.sin_addr.s_addr = inet_addr("127.0.0.1");
    if(INADDR_NONE == sin.sin_addr.s_addr) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "ssh_tunnel_open: inet_addr: %s\n", strerror(errno));
        goto error;
    }
    sinlen = sizeof(sin);
    if(-1 == bind(data->local_listensock, (struct sockaddr *)&sin, sinlen)) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "bind: %s\n", strerror(errno));
        goto error;
    }
    if(-1 == listen(data->local_listensock, 1)) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "listen: %s\n", strerror(errno));
        goto error;
    }

    /* get info back from OS */
    if (getsockname(data->local_listensock, (struct sockaddr *)&sin, &sinlen ) == -1){
        __android_log_print(ANDROID_LOG_ERROR, TAG, "ssh_tunnel_open: getsockname: %s\n", strerror(errno));
        goto error;
    }

    data->local_listenport = ntohs(sin.sin_port);

    __android_log_print(ANDROID_LOG_INFO, TAG,"ssh_tunnel_open: waiting for TCP connection on %s:%d...\n",
           inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));


    /* Create the proxy thread */
    if (pthread_create(&data->thread, NULL, ssh_proxy_loop, data) != 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "ssh_tunnel_open: proxy thread creation failed\n");
        goto error;
    }

    return data;

    error:
    if (data->session) {
        libssh2_session_disconnect(data->session, "Error in SSH tunnel setup");
        libssh2_session_free(data->session);
    }
    if(data->remote_desthost)
        free(data->remote_desthost);

    rfbCloseSocket(data->local_listensock);
    rfbCloseSocket(data->ssh_sock);

    free(data);

    return NULL;
}


void ssh_tunnel_close(SshData *data) {
    if(!data)
        return;

    /* the proxy thread does the internal cleanup as it can be
       ended due to external reasons */
    THREAD_JOIN(data->thread);

    if(data->remote_desthost)
        free(data->remote_desthost);

    free(data);

     __android_log_print(ANDROID_LOG_INFO, TAG,"ssh_tunnel_close: done\n");
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

    // not thread safe according to https://www.libssh2.org/libssh2_init.html
    // so run here once
    if(libssh2_init(0)) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "libssh2 initialization failed\n");
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

/**
   Decide whether or not the SSH tunnel setup should continue
   based on the current host and its fingerprint.
   Business logic is up to the implementer in a real app, i.e.
   compare keys, ask user etc...
   @return -1 if tunnel setup should be aborted
            0 if tunnel setup should continue
 */
static int onSshFingerprintCheck(const char *fingerprint, size_t fingerprint_len,
                                 const char *host, rfbClient *client)
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
        SshData *data;
        if(cSshPassword) {
            // password-based
            data = ssh_tunnel_open(cSshHost, cSshUser, cSshPassword, NULL, 0, NULL, cHost, port, cl);
        } else {
            // key-based
            data = ssh_tunnel_open(cSshHost, cSshUser, NULL, cSshPrivKey, cSshPrivKeyLen, cSshPrivKeyPassword, cHost, port, cl);
        }

        cl->serverHost = strdup("127.0.0.1");
        if(data) // might be NULL if ssh setup failed
            cl->serverPort = data->local_listenport;
        rfbClientSetClientData(cl, VNCCONN_SSH_ID, data);
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