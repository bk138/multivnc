/*
 * Native VNCCanvas drawing.
 * With help from https://www.learnopengles.com/calling-opengl-from-android-using-the-ndk/
 */

#include <jni.h>
#include <GLES/gl.h>
#include "rfb/rfbclient.h"

JNIEXPORT void JNICALL Java_com_coboltforge_dontmind_multivnc_VncCanvas_on_1surface_1created(JNIEnv * env, jclass cls) {
    glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
}

JNIEXPORT void JNICALL Java_com_coboltforge_dontmind_multivnc_VncCanvas_on_1surface_1changed(JNIEnv * env, jclass cls, jint width, jint height) {
    // No-op
}

JNIEXPORT void JNICALL Java_com_coboltforge_dontmind_multivnc_VncCanvas_on_1draw_1frame(JNIEnv * env, jclass cls) {
    glClear(GL_COLOR_BUFFER_BIT);
}

JNIEXPORT void JNICALL Java_com_coboltforge_dontmind_multivnc_VncCanvas_prepareTexture(JNIEnv *env, jclass clazz, jlong rfbClientPtr) {

    rfbClient *client = (rfbClient *) rfbClientPtr;

    if (client && client->frameBuffer) {
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     client->width,
                     client->height,
                     0,
                     GL_RGBA,
                     client->format.depth == 15 ? GL_UNSIGNED_SHORT_5_5_5_1 : GL_UNSIGNED_BYTE,
                     client->frameBuffer);
    }

}

