//
//  VROSceneRendererOVR.cpp
//  ViroRenderer
//
//  Created by Raj Advani on 1/5/17.
//  Copyright © 2017 Viro Media. All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining
//  a copy of this software and associated documentation files (the
//  "Software"), to deal in the Software without restriction, including
//  without limitation the rights to use, copy, modify, merge, publish,
//  distribute, sublicense, and/or sell copies of the Software, and to
//  permit persons to whom the Software is furnished to do so, subject to
//  the following conditions:
//
//  The above copyright notice and this permission notice shall be included
//  in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
//  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "VROSceneRendererOVR.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/prctl.h> // for prctl( PR_SET_NAME )
#include <android/log.h>
#include <android/native_window_jni.h> // for native window JNI
#include <android/input.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include "VROPlatformUtil.h"
#include "VRODriverOpenGLAndroidOVR.h"
#include "VROAllocationTracker.h"
#include "VROInputControllerOVR.h"
#include <VROTime.h>
#include <VrApi_Types.h>
#include <inttypes.h>

#pragma mark - OVR System


#if !defined(EGL_OPENGL_ES3_BIT_KHR)
#define EGL_OPENGL_ES3_BIT_KHR 0x0040
#endif

// EXT_texture_border_clamp
#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER 0x812D
#endif

#ifndef GL_TEXTURE_BORDER_COLOR
#define GL_TEXTURE_BORDER_COLOR 0x1004
#endif

#if !defined(GL_EXT_multisampled_render_to_texture)
typedef void(GL_APIENTRY* PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)(
    GLenum target,
    GLsizei samples,
    GLenum internalformat,
    GLsizei width,
    GLsizei height);
typedef void(GL_APIENTRY* PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)(
    GLenum target,
    GLenum attachment,
    GLenum textarget,
    GLuint texture,
    GLint level,
    GLsizei samples);
#endif

#if !defined(GL_OVR_multiview)
/// static const int GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_NUM_VIEWS_OVR       = 0x9630;
/// static const int GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_BASE_VIEW_INDEX_OVR = 0x9632;
/// static const int GL_MAX_VIEWS_OVR                                      = 0x9631;
typedef void(GL_APIENTRY* PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)(
    GLenum target,
    GLenum attachment,
    GLuint texture,
    GLint level,
    GLint baseViewIndex,
    GLsizei numViews);
#endif

#if !defined(GL_OVR_multiview_multisampled_render_to_texture)
typedef void(GL_APIENTRY* PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC)(
    GLenum target,
    GLenum attachment,
    GLuint texture,
    GLint level,
    GLsizei samples,
    GLint baseViewIndex,
    GLsizei numViews);
#endif

#include "VrApi.h"
#include "VrApi_Helpers.h"
#include "VrApi_SystemUtils.h"
#include "VrApi_Input.h"

#define DEBUG 1
#define OVR_LOG_TAG "VrCubeWorld"

#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, OVR_LOG_TAG, __VA_ARGS__)
#if DEBUG
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, OVR_LOG_TAG, __VA_ARGS__)
#else
#define ALOGV(...)
#endif

static const int CPU_LEVEL = 2;
static const int GPU_LEVEL = 3;
static const int NUM_MULTI_SAMPLES = 4;
static const int OVR_RIGHT_HANDED = 536870915;
static const int OVR_LEFT_HANDED = 536870914;
/*
================================================================================

OVR to VRO Utility Functions

================================================================================
*/

static VROMatrix4f toMatrix4f(ovrMatrix4f in)
{
    float m[16] = {
            in.M[0][0], in.M[1][0], in.M[2][0], in.M[3][0],
            in.M[0][1], in.M[1][1], in.M[2][1], in.M[3][1],
            in.M[0][2], in.M[1][2], in.M[2][2], in.M[3][2],
            in.M[0][3], in.M[1][3], in.M[2][3], in.M[3][3],
    };

    return VROMatrix4f(m);
}

/*
================================================================================

System Clock Time

================================================================================
*/

static double GetTimeInSeconds() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec * 1e9 + now.tv_nsec) * 0.000000001;
}

/*
================================================================================

OpenGL-ES Utility Functions

================================================================================
*/

typedef struct {
    bool multi_view; // GL_OVR_multiview, GL_OVR_multiview2
    bool EXT_texture_border_clamp; // GL_EXT_texture_border_clamp, GL_OES_texture_border_clamp
} OpenGLExtensions_t;

OpenGLExtensions_t glExtensions;

static void EglInitExtensions() {
    const char* allExtensions = (const char*)glGetString(GL_EXTENSIONS);
    if (allExtensions != NULL) {
        glExtensions.multi_view = strstr(allExtensions, "GL_OVR_multiview2") &&
                                  strstr(allExtensions, "GL_OVR_multiview_multisampled_render_to_texture");

        glExtensions.EXT_texture_border_clamp =
                strstr(allExtensions, "GL_EXT_texture_border_clamp") ||
                strstr(allExtensions, "GL_OES_texture_border_clamp");
    }
}

static const char* EglErrorString(const EGLint error) {
    switch (error) {
        case EGL_SUCCESS:
            return "EGL_SUCCESS";
        case EGL_NOT_INITIALIZED:
            return "EGL_NOT_INITIALIZED";
        case EGL_BAD_ACCESS:
            return "EGL_BAD_ACCESS";
        case EGL_BAD_ALLOC:
            return "EGL_BAD_ALLOC";
        case EGL_BAD_ATTRIBUTE:
            return "EGL_BAD_ATTRIBUTE";
        case EGL_BAD_CONTEXT:
            return "EGL_BAD_CONTEXT";
        case EGL_BAD_CONFIG:
            return "EGL_BAD_CONFIG";
        case EGL_BAD_CURRENT_SURFACE:
            return "EGL_BAD_CURRENT_SURFACE";
        case EGL_BAD_DISPLAY:
            return "EGL_BAD_DISPLAY";
        case EGL_BAD_SURFACE:
            return "EGL_BAD_SURFACE";
        case EGL_BAD_MATCH:
            return "EGL_BAD_MATCH";
        case EGL_BAD_PARAMETER:
            return "EGL_BAD_PARAMETER";
        case EGL_BAD_NATIVE_PIXMAP:
            return "EGL_BAD_NATIVE_PIXMAP";
        case EGL_BAD_NATIVE_WINDOW:
            return "EGL_BAD_NATIVE_WINDOW";
        case EGL_CONTEXT_LOST:
            return "EGL_CONTEXT_LOST";
        default:
            return "unknown";
    }
}

static const char* GlFrameBufferStatusString(GLenum status) {
    switch (status) {
        case GL_FRAMEBUFFER_UNDEFINED:
            return "GL_FRAMEBUFFER_UNDEFINED";
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
        case GL_FRAMEBUFFER_UNSUPPORTED:
            return "GL_FRAMEBUFFER_UNSUPPORTED";
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
        default:
            return "unknown";
    }
}

#ifdef CHECK_GL_ERRORS

static const char* GlErrorString(GLenum error) {
    switch (error) {
        case GL_NO_ERROR:
            return "GL_NO_ERROR";
        case GL_INVALID_ENUM:
            return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE:
            return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION:
            return "GL_INVALID_OPERATION";
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            return "GL_INVALID_FRAMEBUFFER_OPERATION";
        case GL_OUT_OF_MEMORY:
            return "GL_OUT_OF_MEMORY";
        default:
            return "unknown";
    }
}

static void GLCheckErrors(int line) {
    for (int i = 0; i < 10; i++) {
        const GLenum error = glGetError();
        if (error == GL_NO_ERROR) {
            break;
        }
        ALOGE("GL error on line %d: %s", line, GlErrorString(error));
    }
}

#define GL(func) \
    func;        \
    GLCheckErrors(__LINE__);

#else // CHECK_GL_ERRORS

#define GL(func) func;

#endif // CHECK_GL_ERRORS

/*
================================================================================

ovrEgl

================================================================================
*/

typedef struct {
    EGLint MajorVersion;
    EGLint MinorVersion;
    EGLDisplay Display;
    EGLConfig Config;
    EGLSurface TinySurface;
    EGLSurface MainSurface;
    EGLContext Context;
} ovrEgl;

static void ovrEgl_Clear(ovrEgl* egl) {
    egl->MajorVersion = 0;
    egl->MinorVersion = 0;
    egl->Display = 0;
    egl->Config = 0;
    egl->TinySurface = EGL_NO_SURFACE;
    egl->MainSurface = EGL_NO_SURFACE;
    egl->Context = EGL_NO_CONTEXT;
}

static void ovrEgl_CreateContext(ovrEgl* egl, const ovrEgl* shareEgl) {
    if (egl->Display != 0) {
        return;
    }

    egl->Display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    ALOGV("        eglInitialize( Display, &MajorVersion, &MinorVersion )");
    eglInitialize(egl->Display, &egl->MajorVersion, &egl->MinorVersion);
    // Do NOT use eglChooseConfig, because the Android EGL code pushes in multisample
    // flags in eglChooseConfig if the user has selected the "force 4x MSAA" option in
    // settings, and that is completely wasted for our warp target.
    const int MAX_CONFIGS = 1024;
    EGLConfig configs[MAX_CONFIGS];
    EGLint numConfigs = 0;
    if (eglGetConfigs(egl->Display, configs, MAX_CONFIGS, &numConfigs) == EGL_FALSE) {
        ALOGE("        eglGetConfigs() failed: %s", EglErrorString(eglGetError()));
        return;
    }
    const EGLint configAttribs[] = {
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8, // need alpha for the multi-pass timewarp compositor
            EGL_DEPTH_SIZE, 0,
            EGL_STENCIL_SIZE, 0,
            EGL_SAMPLES, 0,
            EGL_NONE
    };
    egl->Config = 0;
    for (int i = 0; i < numConfigs; i++) {
        EGLint value = 0;

        eglGetConfigAttrib(egl->Display, configs[i], EGL_RENDERABLE_TYPE, &value);
        if ((value & EGL_OPENGL_ES3_BIT_KHR) != EGL_OPENGL_ES3_BIT_KHR) {
            continue;
        }

        // The pbuffer config also needs to be compatible with normal window rendering
        // so it can share textures with the window context.
        eglGetConfigAttrib(egl->Display, configs[i], EGL_SURFACE_TYPE, &value);
        if ((value & (EGL_WINDOW_BIT | EGL_PBUFFER_BIT)) != (EGL_WINDOW_BIT | EGL_PBUFFER_BIT)) {
            continue;
        }

        int j = 0;
        for (; configAttribs[j] != EGL_NONE; j += 2) {
            eglGetConfigAttrib(egl->Display, configs[i], configAttribs[j], &value);
            if (value != configAttribs[j + 1]) {
                break;
            }
        }
        if (configAttribs[j] == EGL_NONE) {
            egl->Config = configs[i];
            break;
        }
    }
    if (egl->Config == 0) {
        ALOGE("        eglChooseConfig() failed: %s", EglErrorString(eglGetError()));
        return;
    }
    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    ALOGV("        Context = eglCreateContext( Display, Config, EGL_NO_CONTEXT, contextAttribs )");
    egl->Context = eglCreateContext(
            egl->Display,
            egl->Config,
            (shareEgl != NULL) ? shareEgl->Context : EGL_NO_CONTEXT,
            contextAttribs);
    if (egl->Context == EGL_NO_CONTEXT) {
        ALOGE("        eglCreateContext() failed: %s", EglErrorString(eglGetError()));
        return;
    }
    const EGLint surfaceAttribs[] = {EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE};
    ALOGV("        TinySurface = eglCreatePbufferSurface( Display, Config, surfaceAttribs )");
    egl->TinySurface = eglCreatePbufferSurface(egl->Display, egl->Config, surfaceAttribs);
    if (egl->TinySurface == EGL_NO_SURFACE) {
        ALOGE("        eglCreatePbufferSurface() failed: %s", EglErrorString(eglGetError()));
        eglDestroyContext(egl->Display, egl->Context);
        egl->Context = EGL_NO_CONTEXT;
        return;
    }
    ALOGV("        eglMakeCurrent( Display, TinySurface, TinySurface, Context )");
    if (eglMakeCurrent(egl->Display, egl->TinySurface, egl->TinySurface, egl->Context) ==
        EGL_FALSE) {
        ALOGE("        eglMakeCurrent() failed: %s", EglErrorString(eglGetError()));
        eglDestroySurface(egl->Display, egl->TinySurface);
        eglDestroyContext(egl->Display, egl->Context);
        egl->Context = EGL_NO_CONTEXT;
        return;
    }
}

static void ovrEgl_DestroyContext(ovrEgl* egl) {
    if (egl->Display != 0) {
        ALOGE("        eglMakeCurrent( Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT )");
        if (eglMakeCurrent(egl->Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) ==
            EGL_FALSE) {
            ALOGE("        eglMakeCurrent() failed: %s", EglErrorString(eglGetError()));
        }
    }
    if (egl->Context != EGL_NO_CONTEXT) {
        ALOGE("        eglDestroyContext( Display, Context )");
        if (eglDestroyContext(egl->Display, egl->Context) == EGL_FALSE) {
            ALOGE("        eglDestroyContext() failed: %s", EglErrorString(eglGetError()));
        }
        egl->Context = EGL_NO_CONTEXT;
    }
    if (egl->TinySurface != EGL_NO_SURFACE) {
        ALOGE("        eglDestroySurface( Display, TinySurface )");
        if (eglDestroySurface(egl->Display, egl->TinySurface) == EGL_FALSE) {
            ALOGE("        eglDestroySurface() failed: %s", EglErrorString(eglGetError()));
        }
        egl->TinySurface = EGL_NO_SURFACE;
    }
    if (egl->Display != 0) {
        ALOGE("        eglTerminate( Display )");
        if (eglTerminate(egl->Display) == EGL_FALSE) {
            ALOGE("        eglTerminate() failed: %s", EglErrorString(eglGetError()));
        }
        egl->Display = 0;
    }
}

/*
================================================================================

ovrFramebuffer

================================================================================
*/

struct ovrFramebuffer{
    int Width;
    int Height;
    int Multisamples;
    int TextureSwapChainLength;
    int TextureSwapChainIndex;
    bool UseMultiview;
    ovrTextureSwapChain* ColorTextureSwapChain;
    GLuint* DepthBuffers;
    GLuint* FrameBuffers;
};

static void ovrFramebuffer_Clear(ovrFramebuffer* frameBuffer) {
    frameBuffer->Width = 0;
    frameBuffer->Height = 0;
    frameBuffer->Multisamples = 0;
    frameBuffer->TextureSwapChainLength = 0;
    frameBuffer->TextureSwapChainIndex = 0;
    frameBuffer->UseMultiview = false;
    frameBuffer->ColorTextureSwapChain = NULL;
    frameBuffer->DepthBuffers = NULL;
    frameBuffer->FrameBuffers = NULL;
}

static bool ovrFramebuffer_Create(
        ovrFramebuffer* frameBuffer,
        const bool useMultiview,
        const GLenum colorFormat,
        const int width,
        const int height,
        const int multisamples) {
    PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC glRenderbufferStorageMultisampleEXT =
            (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)eglGetProcAddress(
                    "glRenderbufferStorageMultisampleEXT");
    PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC glFramebufferTexture2DMultisampleEXT =
            (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)eglGetProcAddress(
                    "glFramebufferTexture2DMultisampleEXT");

    PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC glFramebufferTextureMultiviewOVR =
            (PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)eglGetProcAddress(
                    "glFramebufferTextureMultiviewOVR");
    PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC glFramebufferTextureMultisampleMultiviewOVR =
            (PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC)eglGetProcAddress(
                    "glFramebufferTextureMultisampleMultiviewOVR");

    frameBuffer->Width = width;
    frameBuffer->Height = height;
    frameBuffer->Multisamples = multisamples;
    frameBuffer->UseMultiview =
            (useMultiview && (glFramebufferTextureMultiviewOVR != NULL)) ? true : false;

    frameBuffer->ColorTextureSwapChain = vrapi_CreateTextureSwapChain3(
            frameBuffer->UseMultiview ? VRAPI_TEXTURE_TYPE_2D_ARRAY : VRAPI_TEXTURE_TYPE_2D,
            colorFormat,
            width,
            height,
            1,
            3);
    frameBuffer->TextureSwapChainLength = vrapi_GetTextureSwapChainLength(frameBuffer->ColorTextureSwapChain);
    frameBuffer->DepthBuffers = (GLuint*)malloc(frameBuffer->TextureSwapChainLength * sizeof(GLuint));
    frameBuffer->FrameBuffers = (GLuint*)malloc(frameBuffer->TextureSwapChainLength * sizeof(GLuint));

    ALOGV("        frameBuffer->UseMultiview = %d", frameBuffer->UseMultiview);

    for (int i = 0; i < frameBuffer->TextureSwapChainLength; i++) {
        // Create the color buffer texture.
        const GLuint colorTexture = vrapi_GetTextureSwapChainHandle(frameBuffer->ColorTextureSwapChain, i);
        GLenum colorTextureTarget = frameBuffer->UseMultiview ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
        GL(glBindTexture(colorTextureTarget, colorTexture));
        if (glExtensions.EXT_texture_border_clamp) {
            GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
            GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));
            GLfloat borderColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
            GL(glTexParameterfv(colorTextureTarget, GL_TEXTURE_BORDER_COLOR, borderColor));
        } else {
            // Just clamp to edge. However, this requires manually clearing the border
            // around the layer to clear the edge texels.
            GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
            GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        }
        GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GL(glBindTexture(colorTextureTarget, 0));

        if (frameBuffer->UseMultiview) {
            // Create the depth buffer texture.
            GL(glGenTextures(1, &frameBuffer->DepthBuffers[i]));
            GL(glBindTexture(GL_TEXTURE_2D_ARRAY, frameBuffer->DepthBuffers[i]));
            GL(glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_DEPTH_COMPONENT24, width, height, 2));
            GL(glBindTexture(GL_TEXTURE_2D_ARRAY, 0));

            // Create the frame buffer.
            GL(glGenFramebuffers(1, &frameBuffer->FrameBuffers[i]));
            GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer->FrameBuffers[i]));
            if (multisamples > 1 && (glFramebufferTextureMultisampleMultiviewOVR != NULL)) {
                GL(glFramebufferTextureMultisampleMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, frameBuffer->DepthBuffers[i], 0 /* level */, multisamples /* samples */, 0 /* baseViewIndex */, 2 /* numViews */));
                GL(glFramebufferTextureMultisampleMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorTexture, 0 /* level */, multisamples /* samples */, 0 /* baseViewIndex */, 2 /* numViews */));
            } else {
                GL(glFramebufferTextureMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, frameBuffer->DepthBuffers[i], 0 /* level */, 0 /* baseViewIndex */, 2 /* numViews */));
                GL(glFramebufferTextureMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorTexture, 0 /* level */, 0 /* baseViewIndex */, 2 /* numViews */));
            }

            GL(GLenum renderFramebufferStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER));
            GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
            if (renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
                ALOGE( "Incomplete frame buffer object: %s", GlFrameBufferStatusString(renderFramebufferStatus));
                return false;
            }
        } else {
            if (multisamples > 1 && glRenderbufferStorageMultisampleEXT != NULL &&
                glFramebufferTexture2DMultisampleEXT != NULL) {
                // Create multisampled depth buffer.
                GL(glGenRenderbuffers(1, &frameBuffer->DepthBuffers[i]));
                GL(glBindRenderbuffer(GL_RENDERBUFFER, frameBuffer->DepthBuffers[i]));
                GL(glRenderbufferStorageMultisampleEXT( GL_RENDERBUFFER, multisamples, GL_DEPTH_COMPONENT24, width, height));
                GL(glBindRenderbuffer(GL_RENDERBUFFER, 0));

                // Create the frame buffer.
                // NOTE: glFramebufferTexture2DMultisampleEXT only works with GL_FRAMEBUFFER.
                GL(glGenFramebuffers(1, &frameBuffer->FrameBuffers[i]));
                GL(glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer->FrameBuffers[i]));
                GL(glFramebufferTexture2DMultisampleEXT( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0, multisamples));
                GL(glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, frameBuffer->DepthBuffers[i]));
                GL(GLenum renderFramebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER));
                GL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
                if (renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
                    ALOGE( "Incomplete frame buffer object: %s", GlFrameBufferStatusString(renderFramebufferStatus));
                    return false;
                }
            } else {
                // Create depth buffer.
                GL(glGenRenderbuffers(1, &frameBuffer->DepthBuffers[i]));
                GL(glBindRenderbuffer(GL_RENDERBUFFER, frameBuffer->DepthBuffers[i]));
                GL(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height));
                GL(glBindRenderbuffer(GL_RENDERBUFFER, 0));

                // Create the frame buffer.
                GL(glGenFramebuffers(1, &frameBuffer->FrameBuffers[i]));
                GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer->FrameBuffers[i]));
                GL(glFramebufferRenderbuffer( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, frameBuffer->DepthBuffers[i]));
                GL(glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0));
                GL(GLenum renderFramebufferStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER));
                GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
                if (renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
                    ALOGE("Incomplete frame buffer object: %s", GlFrameBufferStatusString(renderFramebufferStatus));
                    return false;
                }
            }
        }
    }

    return true;
}

static void ovrFramebuffer_Destroy(ovrFramebuffer* frameBuffer) {
    GL(glDeleteFramebuffers(frameBuffer->TextureSwapChainLength, frameBuffer->FrameBuffers));
    if (frameBuffer->UseMultiview) {
        GL(glDeleteTextures(frameBuffer->TextureSwapChainLength, frameBuffer->DepthBuffers));
    } else {
        GL(glDeleteRenderbuffers(frameBuffer->TextureSwapChainLength, frameBuffer->DepthBuffers));
    }
    vrapi_DestroyTextureSwapChain(frameBuffer->ColorTextureSwapChain);

    free(frameBuffer->DepthBuffers);
    free(frameBuffer->FrameBuffers);

    ovrFramebuffer_Clear(frameBuffer);
}

static void ovrFramebuffer_SetCurrent(ovrFramebuffer* frameBuffer) {
    GL(glBindFramebuffer(
            GL_DRAW_FRAMEBUFFER, frameBuffer->FrameBuffers[frameBuffer->TextureSwapChainIndex]));
}

static void ovrFramebuffer_SetNone() {
    GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
}

static void ovrFramebuffer_Resolve(ovrFramebuffer* frameBuffer) {
    // Discard the depth buffer, so the tiler won't need to write it back out to memory.
    const GLenum depthAttachment[1] = {GL_DEPTH_ATTACHMENT};
    glInvalidateFramebuffer(GL_DRAW_FRAMEBUFFER, 1, depthAttachment);

    // We now let the resolve happen implicitly.
}

static void ovrFramebuffer_Advance(ovrFramebuffer* frameBuffer) {
    // Advance to the next texture from the set.
    frameBuffer->TextureSwapChainIndex =
            (frameBuffer->TextureSwapChainIndex + 1) % frameBuffer->TextureSwapChainLength;
}

/*
================================================================================

ovrSimulation

================================================================================
*/

typedef struct {
    ovrVector3f CurrentRotation;
} ovrSimulation;

static void ovrSimulation_Clear(ovrSimulation* simulation) {
    simulation->CurrentRotation.x = 0.0f;
    simulation->CurrentRotation.y = 0.0f;
    simulation->CurrentRotation.z = 0.0f;
}

static void ovrSimulation_Advance(ovrSimulation* simulation, double elapsedDisplayTime) {
    // Update rotation.
    simulation->CurrentRotation.x = (float)(elapsedDisplayTime);
    simulation->CurrentRotation.y = (float)(elapsedDisplayTime);
    simulation->CurrentRotation.z = (float)(elapsedDisplayTime);
}

/*
================================================================================

ovrRenderer

================================================================================
*/

typedef struct {
    ovrFramebuffer FrameBuffer[VRAPI_FRAME_LAYER_EYE_MAX];
    ovrFramebuffer HUDFrameBuffer[VRAPI_FRAME_LAYER_EYE_MAX];
    int NumBuffers;
} ovrRenderer;

static void ovrRenderer_Clear(ovrRenderer* renderer) {
    for (int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; eye++) {
        ovrFramebuffer_Clear(&renderer->FrameBuffer[eye]);
    }
    renderer->NumBuffers = VRAPI_FRAME_LAYER_EYE_MAX;
}

static void ovrRenderer_Create(ovrRenderer* renderer, const ovrJava* java, const bool useMultiview) {
    renderer->NumBuffers = useMultiview ? 1 : VRAPI_FRAME_LAYER_EYE_MAX;

    // Create the frame buffers.
    for (int eye = 0; eye < renderer->NumBuffers; eye++) {
        ovrFramebuffer_Create(
                &renderer->FrameBuffer[eye],
                useMultiview,
                GL_RGBA8,
                vrapi_GetSystemPropertyInt(java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH),
                vrapi_GetSystemPropertyInt(java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT),
                NUM_MULTI_SAMPLES);

        ovrFramebuffer_Create(
                &renderer->HUDFrameBuffer[eye],
                useMultiview,
                GL_RGBA8,
                vrapi_GetSystemPropertyInt(java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH),
                vrapi_GetSystemPropertyInt(java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT),
                NUM_MULTI_SAMPLES);
    }
}

static void ovrRenderer_Destroy(ovrRenderer* renderer) {
    for (int eye = 0; eye < renderer->NumBuffers; eye++) {
        ovrFramebuffer_Destroy(&renderer->FrameBuffer[eye]);
    }
}

static void ovrRenderer_clearBorder(ovrFramebuffer *frameBuffer) {
    if (glExtensions.EXT_texture_border_clamp == false) {
        // Clear to fully opaque black.
        GL(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
        // bottom
        GL(glScissor(0, 0, frameBuffer->Width, 1));
        GL(glClear(GL_COLOR_BUFFER_BIT));
        // top
        GL(glScissor(0, frameBuffer->Height - 1, frameBuffer->Width, 1));
        GL(glClear(GL_COLOR_BUFFER_BIT));
        // left
        GL(glScissor(0, 0, 1, frameBuffer->Height));
        GL(glClear(GL_COLOR_BUFFER_BIT));
        // right
        GL(glScissor(frameBuffer->Width - 1, 0, 1, frameBuffer->Height));
        GL(glClear(GL_COLOR_BUFFER_BIT));
    }
}

static void ovrRenderer_RenderFrame(ovrRenderer* rendererOVR,
                                                   const ovrJava* java,
                                                   std::shared_ptr<VRORenderer> renderer,
                                                   std::shared_ptr<VRODriverOpenGLAndroid> driver,
                                                   long long frameIndex,
                                                   const ovrSimulation* simulation,
                                                   const ovrTracking2* tracking, ovrMobile* ovr,
                                                   ovrLayerProjection2 *sceneLayer,
                                                   ovrLayerProjection2 *hudLayer) {

    ovrTracking2 updatedTracking = *tracking;

    // Calculate the view matrix.
    ovrPosef headPose = updatedTracking.HeadPose.Pose;
    VROQuaternion quaternion(headPose.Orientation.x, headPose.Orientation.y, headPose.Orientation.z, headPose.Orientation.w);
    VROMatrix4f headRotation = quaternion.getMatrix();

    // The scene layer renders the Scene, the HUD layer renders headlocked UI
    *sceneLayer = vrapi_DefaultLayerProjection2();
    *hudLayer   = vrapi_DefaultLayerProjection2();

    // Ensure the HUD layer is correctly blended on top of the scene layer
    hudLayer->Header.SrcBlend = VRAPI_FRAME_LAYER_BLEND_SRC_ALPHA;
    hudLayer->Header.DstBlend = VRAPI_FRAME_LAYER_BLEND_ONE_MINUS_SRC_ALPHA;

    sceneLayer->HeadPose = updatedTracking.HeadPose;
    hudLayer->HeadPose   = updatedTracking.HeadPose;

    // Improves the quality of the scene layer
    sceneLayer->Header.Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;

    // Ensures "async timewarp" is not applied to the HUD; reduces jitter
    hudLayer->Header.Flags |= VRAPI_FRAME_LAYER_FLAG_FIXED_TO_VIEW;

    VROMatrix4f eyeFromHeadMatrix[VRAPI_FRAME_LAYER_EYE_MAX];
    float interpupillaryDistance = vrapi_GetInterpupillaryDistance(&updatedTracking);

    // Attach each layer to its associated framebuffer's texture swap chain
    for (int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; eye++) {
        ovrFramebuffer* frameBuffer = &rendererOVR->FrameBuffer[rendererOVR->NumBuffers == 1 ? 0 : eye];
        sceneLayer->Textures[eye].ColorSwapChain = frameBuffer->ColorTextureSwapChain;
        sceneLayer->Textures[eye].SwapChainIndex = frameBuffer->TextureSwapChainIndex;
        sceneLayer->Textures[eye].TexCoordsFromTanAngles = ovrMatrix4f_TanAngleMatrixFromProjection(&updatedTracking.Eye[eye].ProjectionMatrix);

        ovrFramebuffer *hudFrameBuffer = &rendererOVR->HUDFrameBuffer[rendererOVR->NumBuffers == 1 ? 0 : eye];
        hudLayer->Textures[eye].ColorSwapChain = hudFrameBuffer->ColorTextureSwapChain;
        hudLayer->Textures[eye].SwapChainIndex = hudFrameBuffer->TextureSwapChainIndex;
        hudLayer->Textures[eye].TexCoordsFromTanAngles = ovrMatrix4f_TanAngleMatrixFromProjection(&updatedTracking.Eye[eye].ProjectionMatrix);

        const float eyeOffset = ( eye ? -0.5f : 0.5f ) * interpupillaryDistance;
        const ovrMatrix4f eyeOffsetMatrix = ovrMatrix4f_CreateTranslation( eyeOffset, 0.0f, 0.0f );
        eyeFromHeadMatrix[eye] = toMatrix4f(eyeOffsetMatrix);
    }

    // Calculate Camera transformations for one eye.
    ovrFramebuffer *leftFB = &rendererOVR->FrameBuffer[0];
    VROViewport leftViewport(0, 0, leftFB->Width, leftFB->Height);
    float fovX = vrapi_GetSystemPropertyFloat( java, VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_X );
    float fovY = vrapi_GetSystemPropertyFloat( java, VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_Y );
    VROFieldOfView fov(fovX / 2.0, fovX / 2.0, fovY / 2.0, fovY / 2.0);
    VROMatrix4f projection = fov.toPerspectiveProjection(kZNear, renderer->getFarClippingPlane());

    VROVector3f headPosition = toMatrix4f(updatedTracking.Eye[0].ViewMatrix).extractTranslation().scale(-1);
    renderer->prepareFrame(frameIndex, leftViewport, fov, headRotation, headPosition, projection,
                           driver);

    // Render the scene to the textures in the scene layer
    for (int eye = 0; eye < rendererOVR->NumBuffers; eye++) {
        // NOTE: In the non-mv case, latency can be further reduced by updating the sensor
        // prediction for each eye (updates orientation, not position)
        ovrFramebuffer* frameBuffer = &rendererOVR->FrameBuffer[eye];
        ovrFramebuffer_SetCurrent(frameBuffer);
        std::dynamic_pointer_cast<VRODisplayOpenGLOVR>(driver->getDisplay())->setFrameBuffer(frameBuffer);

        GL( glEnable(GL_SCISSOR_TEST) );
        GL( glScissor( 0, 0, frameBuffer->Width, frameBuffer->Height) );
        GL( glViewport(0, 0, frameBuffer->Width, frameBuffer->Height) );

        GL( glEnable(GL_DEPTH_TEST) );
        GL( glEnable(GL_STENCIL_TEST) );
        GL( glClearColor(0.0f, 0.0f, 0.0f, 1.0f) );
        GL( glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT) );

        VROEyeType eyeType = (eye == VRAPI_FRAME_LAYER_EYE_LEFT) ? VROEyeType::Left : VROEyeType::Right;
        VROViewport viewport = { 0, 0, frameBuffer->Width, frameBuffer->Height };

        // Recalculate the view matrix with the eye offsets.
        VROMatrix4f ovrEyeView = toMatrix4f(updatedTracking.Eye[eye].ViewMatrix);
        VROVector3f finalPos = ovrEyeView.invert().extractTranslation() + renderer->getCamera().getBasePosition();
        VROVector3f forward = renderer->getCamera().getRotation().getMatrix().multiply(kBaseForward);
        VROVector3f up = renderer->getCamera().getRotation().getMatrix().multiply(kBaseUp);
        VROMatrix4f viroHeadView = VROMathComputeLookAtMatrix(finalPos, forward, up);

        // We use our projection matrix because the one computed by OVR appears to be identical for
        // left and right, but with fixed NCP and FCP. Our projection uses the correct NCP and FCP.
        renderer->renderEye(eyeType,
                            viroHeadView,
                            toMatrix4f(updatedTracking.Eye[eye].ProjectionMatrix),
                            viewport, driver);

        // Explicitly clear the border texels to black when GL_CLAMP_TO_BORDER is not available.
        ovrRenderer_clearBorder(frameBuffer);
        ovrFramebuffer_Resolve(frameBuffer);
        ovrFramebuffer_Advance(frameBuffer);
    }

    // Render the HUD to the textures in the HUD layer
    for (int eye = 0; eye < rendererOVR->NumBuffers; eye++) {
        ovrFramebuffer *frameBuffer = &rendererOVR->HUDFrameBuffer[eye];
        ovrFramebuffer_SetCurrent(frameBuffer);

        GL( glEnable(GL_SCISSOR_TEST) );
        GL( glScissor( 0, 0, frameBuffer->Width, frameBuffer->Height) );
        GL( glViewport(0, 0, frameBuffer->Width, frameBuffer->Height) );

        GL( glEnable(GL_DEPTH_TEST) );
        GL( glClearColor(0.0f, 0.0f, 0.0f, 0.0f) );
        GL( glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT) );

        VROEyeType eyeType = (eye == VRAPI_FRAME_LAYER_EYE_LEFT) ? VROEyeType::Left : VROEyeType::Right;
        renderer->renderHUD(eyeType, eyeFromHeadMatrix[eye], toMatrix4f(updatedTracking.Eye[eye].ProjectionMatrix), driver);

        ovrRenderer_clearBorder(frameBuffer);
        ovrFramebuffer_Resolve(frameBuffer);
        ovrFramebuffer_Advance(frameBuffer);
    }

    renderer->endFrame(driver);
    ALLOCATION_TRACKER_PRINT();

    ovrFramebuffer_SetNone();
}

/*
================================================================================

ovrApp

================================================================================
*/

typedef struct {
    ovrJava Java;
    ovrEgl Egl;
    ANativeWindow* NativeWindow;
    bool Resumed;
    ovrMobile* Ovr;
    ovrSimulation Simulation;
    long long FrameIndex;
    double DisplayTime;
    int SwapInterval;
    int CpuLevel;
    int GpuLevel;
    int MainThreadTid;
    int RenderThreadTid;
    bool BackButtonDownLastFrame;
    bool GamePadBackButtonDown;
    ovrRenderer Renderer;
    bool UseMultiview;

    // Viro parameters
    std::shared_ptr<VRORenderer> vroRenderer;
    std::shared_ptr<VRODriverOpenGLAndroid> driver;
} ovrApp;

static void ovrApp_Clear(ovrApp* app) {
    app->Java.Vm = NULL;
    app->Java.Env = NULL;
    app->Java.ActivityObject = NULL;
    app->NativeWindow = NULL;
    app->Resumed = false;
    app->Ovr = NULL;
    app->FrameIndex = 1;
    app->DisplayTime = 0;
    app->SwapInterval = 1;
    app->CpuLevel = 2;
    app->GpuLevel = 2;
    app->MainThreadTid = 0;
    app->RenderThreadTid = 0;
    app->BackButtonDownLastFrame = false;
    app->GamePadBackButtonDown = false;
    app->UseMultiview = true;

    ovrEgl_Clear(&app->Egl);
    ovrSimulation_Clear(&app->Simulation);
    ovrRenderer_Clear(&app->Renderer);
}

static void ovrApp_HandleVrModeChanges(ovrApp* app) {
    if (app->Resumed != false && app->NativeWindow != NULL) {
        if (app->Ovr == NULL) {
            ovrModeParms parms = vrapi_DefaultModeParms(&app->Java);
            // Must reset the FLAG_FULLSCREEN window flag when using a SurfaceView
            parms.Flags |= VRAPI_MODE_FLAG_RESET_WINDOW_FULLSCREEN;

            parms.Flags |= VRAPI_MODE_FLAG_NATIVE_WINDOW;
            parms.Display = (size_t)app->Egl.Display;
            parms.WindowSurface = (size_t)app->NativeWindow;
            parms.ShareContext = (size_t)app->Egl.Context;

            ALOGV("        eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface(EGL_DRAW));

            ALOGV("        vrapi_EnterVrMode()");

            app->Ovr = vrapi_EnterVrMode(&parms);

            ALOGV("        eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface(EGL_DRAW));

            // If entering VR mode failed then the ANativeWindow was not valid.
            if (app->Ovr == NULL) {
                ALOGE("Invalid ANativeWindow!");
                app->NativeWindow = NULL;
            }

            // Set performance parameters once we have entered VR mode and have a valid ovrMobile.
            if (app->Ovr != NULL) {
                vrapi_SetClockLevels(app->Ovr, app->CpuLevel, app->GpuLevel);

                ALOGV("		vrapi_SetClockLevels( %d, %d )", app->CpuLevel, app->GpuLevel);

                vrapi_SetPerfThread(app->Ovr, VRAPI_PERF_THREAD_TYPE_MAIN, app->MainThreadTid);

                ALOGV("		vrapi_SetPerfThread( MAIN, %d )", app->MainThreadTid);

                vrapi_SetPerfThread(
                        app->Ovr, VRAPI_PERF_THREAD_TYPE_RENDERER, app->RenderThreadTid);

                ALOGV("		vrapi_SetPerfThread( RENDERER, %d )", app->RenderThreadTid);
            }
        }
    } else {
        if (app->Ovr != NULL) {
            ALOGV("        eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface(EGL_DRAW));

            ALOGV("        vrapi_LeaveVrMode()");

            vrapi_LeaveVrMode(app->Ovr);
            app->Ovr = NULL;

            ALOGV("        eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface(EGL_DRAW));
        }
    }
}

static void ovrApp_HandleInput(ovrApp* app, double predictedDisplayTime) {
    bool backButtonDownThisFrame = false;
    std::vector<VROInputControllerOVR::ControllerSnapShot> snapShots;

    for (int i = 0;; i++) {
        ovrInputCapabilityHeader cap;
        ovrResult result = vrapi_EnumerateInputDevices(app->Ovr, i, &cap);
        if (result < 0) {
            break;
        }

        if (cap.Type != ovrControllerType_TrackedRemote) {
            pwarn("Unsupported Controller detected for ViroOculus!");
            return;
        }

        VROInputControllerOVR::ControllerSnapShot snapShot;
        ovrTracking trackingState;
        snapShot.trackingStatus6Dof = false;
        result = vrapi_GetInputTrackingState(app->Ovr, cap.DeviceID, 0, &trackingState);
        if (result == ovrSuccess || result == ovrSuccess_BoundaryInvalid || result == ovrSuccess_EventUnavailable) {
            if (trackingState.Status & VRAPI_TRACKING_STATUS_POSITION_TRACKED &&
                    trackingState.Status & VRAPI_TRACKING_STATUS_POSITION_VALID) {
                snapShot.trackingStatus6Dof = true;
            }
        }

        ovrInputStateTrackedRemote trackedRemoteState;
        trackedRemoteState.Header.ControllerType = ovrControllerType_TrackedRemote;
        result = vrapi_GetCurrentInputState(app->Ovr, cap.DeviceID, &trackedRemoteState.Header);
        snapShot.deviceID = cap.DeviceID;
        snapShot.batteryPercentage = trackedRemoteState.BatteryPercentRemaining;
        pwarn("Controller cap.DeviceID: %" PRIu32"\n snapShot.deviceID %d", cap.DeviceID, snapShot.deviceID);

        if (result == ovrSuccess) {
            // Grab the buttons
            snapShot.buttonAPressed = trackedRemoteState.Buttons & ovrButton_A;
            snapShot.buttonBPressed = trackedRemoteState.Buttons & ovrButton_B;
            snapShot.buttonJoyStickPressed = trackedRemoteState.Buttons & ovrButton_Joystick;
            snapShot.buttonTriggerIndexPressed = trackedRemoteState.Buttons & ovrButton_Trigger;
            snapShot.buttonTrggerHandPressed = trackedRemoteState.Buttons & ovrButton_GripTrigger;
            snapShot.triggerHandWeight = trackedRemoteState.GripTrigger;
            snapShot.triggerIndexWeight = trackedRemoteState.IndexTrigger;
            snapShot.joyStickAxis = VROVector3f(trackedRemoteState.Joystick.x,
                                               trackedRemoteState.Joystick.y, 0);
            snapShot.isConnected = true;
        } else {
            continue;
        }
        // Because OVR makes us do STUPID SHIT TO GRAB A FUCKING POSITION VALUE.
        ovrTracking tracking;
        ovrResult posTrackingResult = vrapi_GetInputTrackingState(app->Ovr,
                cap.DeviceID, predictedDisplayTime, &tracking);
        if (posTrackingResult == ovrSuccess) {
            ovrVector3f pos =  tracking.HeadPose.Pose.Position;
            const ovrQuatf *orientation =  &tracking.HeadPose.Pose.Orientation;
            VROMatrix4f mat = toMatrix4f(ovrMatrix4f_CreateFromQuaternion(orientation));
            VROQuaternion qRot = VROQuaternion(mat);
            snapShot.position = VROVector3f(pos.x, pos.y, pos.z);
            snapShot.rotation = qRot;
        } else {
            continue;
        }

        snapShots.push_back(snapShot);
    }

    // Finally report the state
    std::shared_ptr<VROInputControllerBase> inputControllerBase
            = app->vroRenderer->getInputController();
    inputControllerBase->getPresenter()->updateCamera(app->vroRenderer->getCamera());
    std::shared_ptr<VROInputControllerOVR> inputControllerOvr
            = std::dynamic_pointer_cast<VROInputControllerOVR>(inputControllerBase);
    inputControllerOvr->processInput(snapShots);

    backButtonDownThisFrame |= app->GamePadBackButtonDown;

    bool backButtonDownLastFrame = app->BackButtonDownLastFrame;
    app->BackButtonDownLastFrame = backButtonDownThisFrame;

    if (backButtonDownLastFrame && !backButtonDownThisFrame) {
        ALOGV("back button short press");
        ALOGV("        vrapi_ShowSystemUI( confirmQuit )");
        vrapi_ShowSystemUI(&app->Java, VRAPI_SYS_UI_CONFIRM_QUIT_MENU);
    }
}

static int ovrApp_HandleKeyEvent(ovrApp* app, const int keyCode, const int action) {
    // Handle back button.
    if (keyCode == AKEYCODE_BACK || keyCode == AKEYCODE_BUTTON_B) {
        if (action == AKEY_EVENT_ACTION_DOWN) {
            app->GamePadBackButtonDown = true;
        } else if (action == AKEY_EVENT_ACTION_UP) {
            app->GamePadBackButtonDown = false;
        }
        return 1;
    }
    return 0;
}

static void ovrApp_HandleVrApiEvents(ovrApp* app) {
    ovrEventDataBuffer eventDataBuffer = {};

    // Poll for VrApi events
    for (;;) {
        ovrEventHeader* eventHeader = (ovrEventHeader*)(&eventDataBuffer);
        ovrResult res = vrapi_PollEvent(eventHeader);
        if (res != ovrSuccess) {
            break;
        }

        switch (eventHeader->EventType) {
            case VRAPI_EVENT_DATA_LOST:
                ALOGV("vrapi_PollEvent: Received VRAPI_EVENT_DATA_LOST");
                break;
            case VRAPI_EVENT_VISIBILITY_GAINED:
                ALOGV("vrapi_PollEvent: Received VRAPI_EVENT_VISIBILITY_GAINED");
                break;
            case VRAPI_EVENT_VISIBILITY_LOST:
                ALOGV("vrapi_PollEvent: Received VRAPI_EVENT_VISIBILITY_LOST");
                break;
            case VRAPI_EVENT_FOCUS_GAINED:
                // FOCUS_GAINED is sent when the application is in the foreground and has
                // input focus. This may be due to a system overlay relinquishing focus
                // back to the application.
                ALOGV("vrapi_PollEvent: Received VRAPI_EVENT_FOCUS_GAINED");
                break;
            case VRAPI_EVENT_FOCUS_LOST:
                // FOCUS_LOST is sent when the application is no longer in the foreground and
                // therefore does not have input focus. This may be due to a system overlay taking
                // focus from the application. The application should take appropriate action when
                // this occurs.
                ALOGV("vrapi_PollEvent: Received VRAPI_EVENT_FOCUS_LOST");
                break;
            default:
                ALOGV("vrapi_PollEvent: Unknown event");
                break;
        }
    }
}

/*
================================================================================

ovrMessageQueue

================================================================================
*/

typedef enum {
    MQ_WAIT_NONE, // don't wait
    MQ_WAIT_RECEIVED, // wait until the consumer thread has received the message
    MQ_WAIT_PROCESSED // wait until the consumer thread has processed the message
} ovrMQWait;

#define MAX_MESSAGE_PARMS 8
#define MAX_MESSAGES 1024

typedef struct {
    int Id;
    ovrMQWait Wait;
    long long Parms[MAX_MESSAGE_PARMS];
} ovrMessage;

static void ovrMessage_Init(ovrMessage* message, const int id, const int wait) {
    message->Id = id;
    message->Wait = (ovrMQWait)wait;
    memset(message->Parms, 0, sizeof(message->Parms));
}

static void ovrMessage_SetPointerParm(ovrMessage* message, int index, void* ptr) {
    *(void**)&message->Parms[index] = ptr;
}
static void* ovrMessage_GetPointerParm(ovrMessage* message, int index) {
    return *(void**)&message->Parms[index];
}
static void ovrMessage_SetIntegerParm(ovrMessage* message, int index, int value) {
    message->Parms[index] = value;
}
static int ovrMessage_GetIntegerParm(ovrMessage* message, int index) {
    return (int)message->Parms[index];
}
/// static void		ovrMessage_SetFloatParm( ovrMessage * message, int index, float value ) {
/// *(float *)&message->Parms[index] = value; } static float	ovrMessage_GetFloatParm( ovrMessage
/// * message, int index ) { return *(float *)&message->Parms[index]; }

// Cyclic queue with messages.
typedef struct {
    ovrMessage Messages[MAX_MESSAGES];
    volatile int Head; // dequeue at the head
    volatile int Tail; // enqueue at the tail
    ovrMQWait Wait;
    volatile bool EnabledFlag;
    volatile bool PostedFlag;
    volatile bool ReceivedFlag;
    volatile bool ProcessedFlag;
    pthread_mutex_t Mutex;
    pthread_cond_t PostedCondition;
    pthread_cond_t ReceivedCondition;
    pthread_cond_t ProcessedCondition;
} ovrMessageQueue;

static void ovrMessageQueue_Create(ovrMessageQueue* messageQueue) {
    messageQueue->Head = 0;
    messageQueue->Tail = 0;
    messageQueue->Wait = MQ_WAIT_NONE;
    messageQueue->EnabledFlag = false;
    messageQueue->PostedFlag = false;
    messageQueue->ReceivedFlag = false;
    messageQueue->ProcessedFlag = false;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&messageQueue->Mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    pthread_cond_init(&messageQueue->PostedCondition, NULL);
    pthread_cond_init(&messageQueue->ReceivedCondition, NULL);
    pthread_cond_init(&messageQueue->ProcessedCondition, NULL);
}

static void ovrMessageQueue_Destroy(ovrMessageQueue* messageQueue) {
    pthread_mutex_destroy(&messageQueue->Mutex);
    pthread_cond_destroy(&messageQueue->PostedCondition);
    pthread_cond_destroy(&messageQueue->ReceivedCondition);
    pthread_cond_destroy(&messageQueue->ProcessedCondition);
}

static void ovrMessageQueue_Enable(ovrMessageQueue* messageQueue, const bool set) {
    messageQueue->EnabledFlag = set;
}

static void ovrMessageQueue_PostMessage(ovrMessageQueue* messageQueue, const ovrMessage* message) {
    if (!messageQueue->EnabledFlag) {
        return;
    }
    while (messageQueue->Tail - messageQueue->Head >= MAX_MESSAGES) {
        usleep(1000);
    }
    pthread_mutex_lock(&messageQueue->Mutex);
    messageQueue->Messages[messageQueue->Tail & (MAX_MESSAGES - 1)] = *message;
    messageQueue->Tail++;
    messageQueue->PostedFlag = true;
    pthread_cond_broadcast(&messageQueue->PostedCondition);
    if (message->Wait == MQ_WAIT_RECEIVED) {
        while (!messageQueue->ReceivedFlag) {
            pthread_cond_wait(&messageQueue->ReceivedCondition, &messageQueue->Mutex);
        }
        messageQueue->ReceivedFlag = false;
    } else if (message->Wait == MQ_WAIT_PROCESSED) {
        while (!messageQueue->ProcessedFlag) {
            pthread_cond_wait(&messageQueue->ProcessedCondition, &messageQueue->Mutex);
        }
        messageQueue->ProcessedFlag = false;
    }
    pthread_mutex_unlock(&messageQueue->Mutex);
}

static void ovrMessageQueue_SleepUntilMessage(ovrMessageQueue* messageQueue) {
    if (messageQueue->Wait == MQ_WAIT_PROCESSED) {
        messageQueue->ProcessedFlag = true;
        pthread_cond_broadcast(&messageQueue->ProcessedCondition);
        messageQueue->Wait = MQ_WAIT_NONE;
    }
    pthread_mutex_lock(&messageQueue->Mutex);
    if (messageQueue->Tail > messageQueue->Head) {
        pthread_mutex_unlock(&messageQueue->Mutex);
        return;
    }
    while (!messageQueue->PostedFlag) {
        pthread_cond_wait(&messageQueue->PostedCondition, &messageQueue->Mutex);
    }
    messageQueue->PostedFlag = false;
    pthread_mutex_unlock(&messageQueue->Mutex);
}

static bool ovrMessageQueue_GetNextMessage(
        ovrMessageQueue* messageQueue,
        ovrMessage* message,
        bool waitForMessages) {
    if (messageQueue->Wait == MQ_WAIT_PROCESSED) {
        messageQueue->ProcessedFlag = true;
        pthread_cond_broadcast(&messageQueue->ProcessedCondition);
        messageQueue->Wait = MQ_WAIT_NONE;
    }
    if (waitForMessages) {
        ovrMessageQueue_SleepUntilMessage(messageQueue);
    }
    pthread_mutex_lock(&messageQueue->Mutex);
    if (messageQueue->Tail <= messageQueue->Head) {
        pthread_mutex_unlock(&messageQueue->Mutex);
        return false;
    }
    *message = messageQueue->Messages[messageQueue->Head & (MAX_MESSAGES - 1)];
    messageQueue->Head++;
    pthread_mutex_unlock(&messageQueue->Mutex);
    if (message->Wait == MQ_WAIT_RECEIVED) {
        messageQueue->ReceivedFlag = true;
        pthread_cond_broadcast(&messageQueue->ReceivedCondition);
    } else if (message->Wait == MQ_WAIT_PROCESSED) {
        messageQueue->Wait = MQ_WAIT_PROCESSED;
    }
    return true;
}

/*
================================================================================

ovrAppThread

================================================================================
*/

enum {
    MESSAGE_ON_CREATE,
    MESSAGE_ON_START,
    MESSAGE_ON_RESUME,
    MESSAGE_ON_PAUSE,
    MESSAGE_ON_STOP,
    MESSAGE_ON_DESTROY,
    MESSAGE_ON_SURFACE_CREATED,
    MESSAGE_ON_SURFACE_DESTROYED,
    MESSAGE_ON_KEY_EVENT,
    MESSAGE_ON_TOUCH_EVENT,
};

struct ovrAppThread {
    JavaVM* JavaVm;
    jobject ActivityObject;
    pthread_t Thread;
    ovrMessageQueue MessageQueue;
    ANativeWindow* NativeWindow;

    std::shared_ptr<VRORenderer> vroRenderer;
    std::shared_ptr<VRODriverOpenGLAndroid> driver;
    jobject view;
};

void* AppThreadFunction(void* parm) {
    ovrAppThread* appThread = (ovrAppThread*)parm;

    ovrJava java;
    java.Vm = appThread->JavaVm;
    java.Vm->AttachCurrentThread( &java.Env, NULL );
    java.ActivityObject = appThread->ActivityObject;

    // Note that AttachCurrentThread will reset the thread name.
    prctl(PR_SET_NAME, (long)"OVR::Main", 0, 0, 0);

    const ovrInitParms initParms = vrapi_DefaultInitParms(&java);
    int32_t initResult = vrapi_Initialize(&initParms);
    if (initResult != VRAPI_INITIALIZE_SUCCESS) {
        // If intialization failed, vrapi_* function calls will not be available.
        // VIRO TODO: Call vrapi_Shutdown?
        exit(0);
    }

    ovrApp appState;
    ovrApp_Clear(&appState);
    appState.Java = java;

    // This app will handle android gamepad events itself.
    vrapi_SetPropertyInt(&appState.Java, VRAPI_EAT_NATIVE_GAMEPAD_EVENTS, 0);

    ovrEgl_CreateContext(&appState.Egl, NULL);

    EglInitExtensions();

    appState.UseMultiview &=
            (glExtensions.multi_view &&
             vrapi_GetSystemPropertyInt(&appState.Java, VRAPI_SYS_PROP_MULTIVIEW_AVAILABLE));

    // TODO VIRO-725: enable multiview rendering (start by removing this line to detect multiview support correctly)
    appState.UseMultiview = false;

    ALOGV("AppState UseMultiview : %d", appState.UseMultiview);

    appState.CpuLevel = CPU_LEVEL;
    appState.GpuLevel = GPU_LEVEL;
    appState.MainThreadTid = gettid();
    ovrRenderer_Create(&appState.Renderer, &java, appState.UseMultiview);
    appState.vroRenderer = appThread->vroRenderer;
    appState.driver = appThread->driver;

    jclass viewCls = java.Env->GetObjectClass(appThread->view);
    jmethodID drawFrameMethod = java.Env->GetMethodID(viewCls, "onDrawFrame", "()V");

    const double startTime = GetTimeInSeconds();

    for (bool destroyed = false; destroyed == false;) {
        for (;;) {
            ovrMessage message;
            const bool waitForMessages = (appState.Ovr == NULL && destroyed == false);
            if (!ovrMessageQueue_GetNextMessage(
                    &appThread->MessageQueue, &message, waitForMessages)) {
                break;
            }

            switch (message.Id) {
                case MESSAGE_ON_CREATE: {
                    break;
                }
                case MESSAGE_ON_START: {
                    break;
                }
                case MESSAGE_ON_RESUME: {
                    appState.Resumed = true;
                    break;
                }
                case MESSAGE_ON_PAUSE: {
                    appState.Resumed = false;
                    break;
                }
                case MESSAGE_ON_STOP: {
                    break;
                }
                case MESSAGE_ON_DESTROY: {
                    appState.NativeWindow = NULL;
                    destroyed = true;
                    break;
                }
                case MESSAGE_ON_SURFACE_CREATED: {
                    appState.NativeWindow = (ANativeWindow*)ovrMessage_GetPointerParm(&message, 0);
                    break;
                }
                case MESSAGE_ON_SURFACE_DESTROYED: {
                    appState.NativeWindow = NULL;
                    break;
                }
                case MESSAGE_ON_KEY_EVENT: {
                    ovrApp_HandleKeyEvent(
                            &appState,
                            ovrMessage_GetIntegerParm(&message, 0),
                            ovrMessage_GetIntegerParm(&message, 1));
                    break;
                }
            }

            ovrApp_HandleVrModeChanges(&appState);
        }

        // We must read from the event queue with regular frequency.
        ovrApp_HandleVrApiEvents(&appState);

        // Invoke the frame listeners on the Java side
        java.Env->CallVoidMethod(appThread->view, drawFrameMethod);

        if (appState.Ovr == NULL) {
            continue;
        }

        // This is the only place the frame index is incremented, right before
        // calling vrapi_GetPredictedDisplayTime().
        appState.FrameIndex++;

        // Get the HMD pose, predicted for the middle of the time period during which
        // the new eye images will be displayed. The number of frames predicted ahead
        // depends on the pipeline depth of the engine and the synthesis rate.
        // The better the prediction, the less black will be pulled in at the edges.
        const double predictedDisplayTime =
                vrapi_GetPredictedDisplayTime(appState.Ovr, appState.FrameIndex);
        const ovrTracking2 tracking =
                vrapi_GetPredictedTracking2(appState.Ovr, predictedDisplayTime);

        appState.DisplayTime = predictedDisplayTime;
        ovrApp_HandleInput(&appState, predictedDisplayTime);

        // Advance the simulation based on the elapsed time since start of loop till predicted
        // display time.
        ovrSimulation_Advance(&appState.Simulation, predictedDisplayTime - startTime);

        // Render eye images and setup the primary layer using ovrTracking2.
        ovrLayerProjection2 worldLayer;
        ovrLayerProjection2 hudLayer;
        ovrRenderer_RenderFrame(
                &appState.Renderer,
                &appState.Java,
                appState.vroRenderer,
                appState.driver,
                appState.FrameIndex,
                &appState.Simulation,
                &tracking, appState.Ovr, &worldLayer, &hudLayer);

        const ovrLayerHeader2 * layers[] = {
                &worldLayer.Header,
                &hudLayer.Header
        };

        ovrSubmitFrameDescription2 frameDesc = {};
        frameDesc.Flags = 0;
        frameDesc.SwapInterval = appState.SwapInterval;
        frameDesc.FrameIndex = appState.FrameIndex;
        frameDesc.DisplayTime = appState.DisplayTime;
        frameDesc.LayerCount = 2;
        frameDesc.Layers = layers;

        // Hand over the eye images to the time warp.
        vrapi_SubmitFrame2( appState.Ovr, &frameDesc );
    }

    ovrRenderer_Destroy(&appState.Renderer);
    ovrEgl_DestroyContext(&appState.Egl);
    vrapi_Shutdown();

    java.Vm->DetachCurrentThread();
    return NULL;
}

static void ovrAppThread_Create(ovrAppThread* appThread, JNIEnv* env, jobject activityObject, jobject viewObject,
                                std::shared_ptr<VRORenderer> renderer, std::shared_ptr<VRODriverOpenGLAndroid> driver) {
    env->GetJavaVM( &appThread->JavaVm );
    appThread->ActivityObject = env->NewGlobalRef(activityObject);
    appThread->Thread = 0;
    appThread->NativeWindow = NULL;
    appThread->view = env->NewGlobalRef(viewObject);
    appThread->vroRenderer = renderer;
    appThread->driver = driver;
    ovrMessageQueue_Create(&appThread->MessageQueue);

    const int createErr = pthread_create(&appThread->Thread, NULL, AppThreadFunction, appThread);
    if (createErr != 0) {
        ALOGE("pthread_create returned %i", createErr);
    }
}

static void ovrAppThread_Destroy(ovrAppThread* appThread, JNIEnv* env) {
    pthread_join(appThread->Thread, NULL);
    env->DeleteGlobalRef( appThread->ActivityObject );
    env->DeleteGlobalRef( appThread->view );
    appThread->vroRenderer.reset();
    appThread->driver.reset();
    ovrMessageQueue_Destroy(&appThread->MessageQueue);
}

/*
================================================================================

Activity lifecycle

================================================================================
*/

VROSceneRendererOVR::VROSceneRendererOVR(VRORendererConfiguration config,
                                                 std::shared_ptr<gvr::AudioApi> gvrAudio,
                                                 jobject view, jobject activity, JNIEnv *env) {
    _driver = std::make_shared<VRODriverOpenGLAndroidOVR>(gvrAudio);
    _renderer = std::make_shared<VRORenderer>(config, std::make_shared<VROInputControllerOVR>(_driver));

    ALOGV( "    GLES3JNILib::onCreate()" );

    _appThread = (ovrAppThread *) malloc( sizeof( ovrAppThread ) );
    ovrAppThread_Create( _appThread, env, activity, view, _renderer, _driver );

    ovrMessageQueue_Enable( &_appThread->MessageQueue, true );
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_CREATE, MQ_WAIT_PROCESSED );
    ovrMessageQueue_PostMessage( &_appThread->MessageQueue, &message );
}

VROSceneRendererOVR::~VROSceneRendererOVR() {
}

void VROSceneRendererOVR::onStart() {
    ALOGV( "    GLES3JNILib::onStart()" );
    ovrAppThread * appThread = _appThread;
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_START, MQ_WAIT_PROCESSED );
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

void VROSceneRendererOVR::onResume() {
    ALOGV( "    GLES3JNILib::onResume()" );
    ovrAppThread * appThread = _appThread;
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_RESUME, MQ_WAIT_PROCESSED );
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

void VROSceneRendererOVR::onPause() {
    ALOGV( "    GLES3JNILib::onPause()" );
    ovrAppThread * appThread = _appThread;
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_PAUSE, MQ_WAIT_PROCESSED );
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

void VROSceneRendererOVR::onStop() {
    ALOGV( "    GLES3JNILib::onStop()" );
    ovrAppThread * appThread = _appThread;
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_STOP, MQ_WAIT_PROCESSED );
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

void VROSceneRendererOVR::onDestroy(){
    ALOGV( "    GLES3JNILib::onDestroy()" );
    JNIEnv *env = VROPlatformGetJNIEnv();
    ovrAppThread * appThread = _appThread;
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_DESTROY, MQ_WAIT_PROCESSED );
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
    ovrMessageQueue_Enable( &appThread->MessageQueue, false );

    ovrAppThread_Destroy( appThread, env );
    free( appThread );
}

/*
================================================================================

Surface lifecycle

================================================================================
*/
void VROSceneRendererOVR::onSurfaceCreated(jobject surface) {
    ALOGV( "    GLES3JNILib::onSurfaceCreated()" );
    ovrAppThread * appThread = _appThread;

    JNIEnv *env = VROPlatformGetJNIEnv();

    ANativeWindow * newNativeWindow = ANativeWindow_fromSurface( env, surface );
    if ( ANativeWindow_getWidth( newNativeWindow ) < ANativeWindow_getHeight( newNativeWindow ) )
    {
        // An app that is relaunched after pressing the home button gets an initial surface with
        // the wrong orientation even though android:screenOrientation="landscape" is set in the
        // manifest. The choreographer callback will also never be called for this surface because
        // the surface is immediately replaced with a new surface with the correct orientation.
        ALOGE( "        Surface not in landscape mode!" );
    }

    ALOGV( "        NativeWindow = ANativeWindow_fromSurface( env, surface )" );
    appThread->NativeWindow = newNativeWindow;
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_SURFACE_CREATED, MQ_WAIT_PROCESSED );
    ovrMessage_SetPointerParm( &message, 0, appThread->NativeWindow );
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

void VROSceneRendererOVR::onSurfaceChanged(jobject surface, VRO_INT width, VRO_INT height) {
    ALOGV( "    GLES3JNILib::onSurfaceChanged()" );
    ovrAppThread * appThread = _appThread;

    JNIEnv *env = VROPlatformGetJNIEnv();

    ANativeWindow * newNativeWindow = ANativeWindow_fromSurface( env, surface );
    if ( ANativeWindow_getWidth( newNativeWindow ) < ANativeWindow_getHeight( newNativeWindow ) )
    {
        // An app that is relaunched after pressing the home button gets an initial surface with
        // the wrong orientation even though android:screenOrientation="landscape" is set in the
        // manifest. The choreographer callback will also never be called for this surface because
        // the surface is immediately replaced with a new surface with the correct orientation.
        ALOGE( "        Surface not in landscape mode!" );
    }

    if ( newNativeWindow != appThread->NativeWindow )
    {
        if ( appThread->NativeWindow != NULL )
        {
            ovrMessage message;
            ovrMessage_Init( &message, MESSAGE_ON_SURFACE_DESTROYED, MQ_WAIT_PROCESSED );
            ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
            ALOGV( "        ANativeWindow_release( NativeWindow )" );
            ANativeWindow_release( appThread->NativeWindow );
            appThread->NativeWindow = NULL;
        }
        if ( newNativeWindow != NULL )
        {
            ALOGV( "        NativeWindow = ANativeWindow_fromSurface( env, surface )" );
            appThread->NativeWindow = newNativeWindow;
            ovrMessage message;
            ovrMessage_Init( &message, MESSAGE_ON_SURFACE_CREATED, MQ_WAIT_PROCESSED );
            ovrMessage_SetPointerParm( &message, 0, appThread->NativeWindow );
            ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
        }
    }
    else if ( newNativeWindow != NULL )
    {
        ANativeWindow_release( newNativeWindow );
    }
}

void VROSceneRendererOVR::onSurfaceDestroyed() {
    ALOGV( "    GLES3JNILib::onSurfaceDestroyed()" );
    ovrAppThread * appThread = _appThread;
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_SURFACE_DESTROYED, MQ_WAIT_PROCESSED );
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
    ALOGV( "        ANativeWindow_release( NativeWindow )" );
    ANativeWindow_release( appThread->NativeWindow );
    appThread->NativeWindow = NULL;
}

/*
================================================================================

Input

================================================================================
*/

void VROSceneRendererOVR::onKeyEvent(int keyCode, int action) {
    if ( action == AKEY_EVENT_ACTION_UP )
    {
        ALOGV( "    GLES3JNILib::onKeyEvent( %d, %d )", keyCode, action );
    }
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_KEY_EVENT, MQ_WAIT_NONE );
    ovrMessage_SetIntegerParm( &message, 0, keyCode );
    ovrMessage_SetIntegerParm( &message, 1, action );
    ovrMessageQueue_PostMessage( &_appThread->MessageQueue, &message );
}

void VROSceneRendererOVR::onTouchEvent(int action, float x, float y) {
    // No-op
}

void VROSceneRendererOVR::recenterTracking() {
    // No-op
};
