//
// Created by parallelno on 02.04.2016.
//

#include "common.hpp"
#include "FGEngine.hpp"
#include "Asset.h"

// verbose debug logs on?
#define VERBOSE_LOGGING 1

#if VERBOSE_LOGGING
    #define VLOGD LOGD
#else
    #define VLOGD
#endif

// max # of GL errors to print before giving up
#define MAX_GL_ERRORS 200

static FGEngine *_singleton = NULL;

FGEngine::FGEngine(struct android_app *app) {
    const char path[] = "test.txt";
    Asset *asset = new Asset(app, path);
	bool isOpen = asset->open();
	LOGI("Is open %d", isOpen);
	char text[7];
	LOGI("new buff");
	memset(text, 0, sizeof(text));
	LOGI("clear bytes; %d", sizeof(text));
	bool isReaded = asset->read(text, 6);
	LOGI("Is readed %d", isReaded);

    LOGI("File Text.txt: %s", text);
	asset->close();
	LOGI("File closed");

    LOGD("NativeEngine: initializing.");
    mApp = app;
    mHasFocus = mIsVisible = mHasWindow = false;
    mHasGLObjects = false;
    mEglDisplay = EGL_NO_DISPLAY;
    mEglSurface = EGL_NO_SURFACE;
    mEglContext = EGL_NO_CONTEXT;
    mEglConfig = 0;
    mSurfWidth = mSurfHeight = 0;
    mApiVersion = 0;
    mJniEnv = NULL;
    memset(&mState, 0, sizeof(mState));
    mIsFirstFrame = true;

    if (app->savedState != NULL) {
        // we are starting with previously saved state -- restore it
        mState = *(struct NativeEngineSavedState*) app->savedState;
    }

    // only one instance of NativeEngine may exist!
    MY_ASSERT(_singleton == NULL);
    _singleton = this;

    LOGD("NativeEngine: API version %d.", mApiVersion);
}

FGEngine::~FGEngine() {
    KillContext();
    if (mJniEnv) {
        LOGD("Detaching current thread from JNI.");
        mApp->activity->vm->DetachCurrentThread();
        LOGD("Current thread detached from JNI.");
        mJniEnv = NULL;
    }
    _singleton = NULL;
}

static void _handle_cmd_proxy(struct android_app* app, int32_t cmd) {
    FGEngine *engine = (FGEngine*) app->userData;
    engine->HandleCommand(cmd);
}

static int _handle_input_proxy(struct android_app* app, AInputEvent* event) {
    FGEngine *engine = (FGEngine*) app->userData;
    return engine->HandleInput(event) ? 1 : 0;
}

void FGEngine::GameLoop() {
    mApp->userData = this;
    mApp->onAppCmd = _handle_cmd_proxy;
    mApp->onInputEvent = _handle_input_proxy;

    while (1) {
        int ident, events;
        struct android_poll_source* source;

        // If not animating, block until we get an event; if animating, don't block.
        while ((ident = ALooper_pollAll(IsAnimating() ? 0 : -1, NULL, &events,
                                        (void**)&source)) >= 0) {

            // process event
            if (source != NULL) {
                source->process(mApp, source);
            }

            // are we exiting?
            if (mApp->destroyRequested) {
                return;
            }
        }

        if (IsAnimating()) {
            DoFrame();
        }
    }
}

void FGEngine::KillContext() {
    LOGD("NativeEngine: killing context.");

    // since the context is going away, we have to kill the GL objects
    //KillGLObjects();

    eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    if (mEglContext != EGL_NO_CONTEXT) {
        eglDestroyContext(mEglDisplay, mEglContext);
        mEglContext = EGL_NO_CONTEXT;
    }
    LOGD("NativeEngine: Context killed successfully.");
}

void FGEngine::KillSurface() {
    LOGD("NativeEngine: killing surface.");
    eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (mEglSurface != EGL_NO_SURFACE) {
        eglDestroySurface(mEglDisplay, mEglSurface);
        mEglSurface = EGL_NO_SURFACE;
    }
    LOGD("NativeEngine: Surface killed successfully.");
}


void FGEngine::HandleCommand(int32_t cmd) {
    //SceneManager *mgr = SceneManager::GetInstance();

    //LOGD("NativeEngine: handling command %d.", cmd);
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            // The system has asked us to save our current state.
            VLOGD("NativeEngine: APP_CMD_SAVE_STATE");
            mApp->savedState = malloc(sizeof(mState));
            *((NativeEngineSavedState*)mApp->savedState) = mState;
            mApp->savedStateSize = sizeof(mState);
            break;
        case APP_CMD_INIT_WINDOW:
            // We have a window!
            VLOGD("NativeEngine: APP_CMD_INIT_WINDOW");
            if (mApp->window != NULL) {
                mHasWindow = true;
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is going away -- kill the surface
            VLOGD("NativeEngine: APP_CMD_TERM_WINDOW");
            KillSurface();
            mHasWindow = false;
            break;
        case APP_CMD_GAINED_FOCUS:
            VLOGD("NativeEngine: APP_CMD_GAINED_FOCUS");
            mHasFocus = true;
            break;
        case APP_CMD_LOST_FOCUS:
            VLOGD("NativeEngine: APP_CMD_LOST_FOCUS");
            mHasFocus = false;
            break;
        case APP_CMD_PAUSE:
            VLOGD("NativeEngine: APP_CMD_PAUSE");
            //mgr->OnPause();
            break;
        case APP_CMD_RESUME:
            VLOGD("NativeEngine: APP_CMD_RESUME");
            //mgr->OnResume();
            break;
        case APP_CMD_STOP:
            VLOGD("NativeEngine: APP_CMD_STOP");
            mIsVisible = false;
            break;
        case APP_CMD_START:
            VLOGD("NativeEngine: APP_CMD_START");
            mIsVisible = true;
            break;
        case APP_CMD_WINDOW_RESIZED:
        case APP_CMD_CONFIG_CHANGED:
            VLOGD("NativeEngine: %s", cmd == APP_CMD_WINDOW_RESIZED ?
                                      "APP_CMD_WINDOW_RESIZED" : "APP_CMD_CONFIG_CHANGED");
            // Window was resized or some other configuration changed.
            // Note: we don't handle this event because we check the surface dimensions
            // every frame, so that's how we know it was resized. If you are NOT doing that,
            // then you need to handle this event!
            break;
        case APP_CMD_LOW_MEMORY:
            VLOGD("NativeEngine: APP_CMD_LOW_MEMORY");
            // system told us we have low memory. So if we are not visible, let's
            // cooperate by deallocating all of our graphic resources.
            if (!mHasWindow) {
                VLOGD("NativeEngine: trimming memory footprint (deleting GL objects).");
                //KillGLObjects();
            }
            break;
        default:
            VLOGD("NativeEngine: (unknown command).");
            break;
    }

    VLOGD("NativeEngine: STATUS: F%d, V%d, W%d, EGL: D %p, S %p, CTX %p, CFG %p",
          mHasFocus, mIsVisible, mHasWindow, mEglDisplay, mEglSurface, mEglContext, mEglConfig);
}

bool FGEngine::HandleInput(AInputEvent *event) {
    return 0;
}

bool FGEngine::IsAnimating() {
    return mHasFocus && mIsVisible && mHasWindow;;
}

static const char *_log_opengl_error(GLenum err) {
    switch (err) {
        case GL_NO_ERROR:
            LOGE("*** OpenGL error: GL_NO_ERROR");
            break;
        case GL_INVALID_ENUM:
            LOGE("*** OpenGL error: GL_INVALID_ENUM");
            break;
        case GL_INVALID_VALUE:
            LOGE("*** OpenGL error: GL_INVALID_VALUE");
            break;
        case GL_INVALID_OPERATION:
            LOGE("*** OpenGL error: GL_INVALID_OPERATION");
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            LOGE("*** OpenGL error: GL_INVALID_FRAMEBUFFER_OPERATION");
            break;
        case GL_OUT_OF_MEMORY:
            LOGE("*** OpenGL error: GL_OUT_OF_MEMORY");
            break;
        default:
            LOGE("*** OpenGL error: error %d", err);
            break;
    }
}

void FGEngine::DoFrame() {
    // prepare to render (create context, surfaces, etc, if needed)
    if (!PrepareToRender()) {
        // not ready
                VLOGD("NativeEngine: preparation to render failed.");
        return;
    }

//    SceneManager *mgr = SceneManager::GetInstance();

    // how big is the surface? We query every frame because it's cheap, and some
    // strange devices out there change the surface size without calling any callbacks...
    int width, height;
    eglQuerySurface(mEglDisplay, mEglSurface, EGL_WIDTH, &width);
    eglQuerySurface(mEglDisplay, mEglSurface, EGL_HEIGHT, &height);

    if (width != mSurfWidth || height != mSurfHeight) {
        // notify scene manager that the surface has changed size
        LOGD("NativeEngine: surface changed size %dx%d --> %dx%d", mSurfWidth, mSurfHeight,
             width, height);
        mSurfWidth = width;
        mSurfHeight = height;
//        mgr->SetScreenSize(mSurfWidth, mSurfHeight);
        glViewport(0, 0, mSurfWidth, mSurfHeight);
    }
/*
    // if this is the first frame, install the welcome scene
    if (mIsFirstFrame) {
        mIsFirstFrame = false;
        mgr->RequestNewScene(new WelcomeScene());
    }
*/
    // render!
//    mgr->DoFrame();

    // swap buffers
    if (EGL_FALSE == eglSwapBuffers(mEglDisplay, mEglSurface)) {
        // failed to swap buffers...
        LOGW("NativeEngine: eglSwapBuffers failed, EGL error %d", eglGetError());
        HandleEglError(eglGetError());
    }

    // print out GL errors, if any
    GLenum e;
    static int errorsPrinted = 0;
    while ((e = glGetError()) != GL_NO_ERROR) {
        if (errorsPrinted < MAX_GL_ERRORS) {
            _log_opengl_error(e);
            ++errorsPrinted;
            if (errorsPrinted >= MAX_GL_ERRORS) {
                LOGE("*** NativeEngine: TOO MANY OPENGL ERRORS. NO LONGER PRINTING.");
            }
        }
    }
}

bool FGEngine::PrepareToRender() {
    do {
        // if we're missing a surface, context, or display, create them
        if (mEglDisplay == EGL_NO_DISPLAY || mEglSurface == EGL_NO_SURFACE ||
            mEglContext == EGL_NO_CONTEXT) {

            // create display if needed
            if (!InitDisplay()) {
                LOGE("NativeEngine: failed to create display.");
                return false;
            }

            // create surface if needed
            if (!InitSurface()) {
                LOGE("NativeEngine: failed to create surface.");
                return false;
            }

            // create context if needed
            if (!InitContext()) {
                LOGE("NativeEngine: failed to create context.");
                return false;
            }

            LOGD("NativeEngine: binding surface and context (display %p, surface %p, context %p)",
                 mEglDisplay, mEglSurface, mEglContext);

            // bind them
            if (EGL_FALSE == eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext)) {
                LOGE("NativeEngine: eglMakeCurrent failed, EGL error %d", eglGetError());
                HandleEglError(eglGetError());
            }

            // configure our global OpenGL settings
            ConfigureOpenGL();
        }
/*
        // now that we're sure we have a context and all, if we don't have the OpenGL
        // objects ready, create them.
        if (!mHasGLObjects) {
            LOGD("NativeEngine: creating OpenGL objects.");
            if (!InitGLObjects()) {
                LOGE("NativeEngine: unable to initialize OpenGL objects.");
                return false;
            }
        }
*/
    } while(0);

    // ready to render
    return true;
}

bool FGEngine::InitDisplay() {
    if (mEglDisplay != EGL_NO_DISPLAY) {
        // nothing to do
        LOGD("NativeEngine: no need to init display (already had one).");
        return true;
    }

    LOGD("NativeEngine: initializing display.");
    mEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (EGL_FALSE == eglInitialize(mEglDisplay, 0, 0)) {
        LOGE("NativeEngine: failed to init display, error %d", eglGetError());
        return false;
    }
    return true;
}

bool FGEngine::InitSurface() {
    // need a display
    MY_ASSERT(mEglDisplay != EGL_NO_DISPLAY);

    if (mEglSurface != EGL_NO_SURFACE) {
        // nothing to do
        LOGD("NativeEngine: no need to init surface (already had one).");
        return true;
    }

    LOGD("NativeEngine: initializing surface.");

    EGLint numConfigs;

    const EGLint attribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, // request OpenGL ES 2.0
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_DEPTH_SIZE, 16,
            EGL_NONE
    };

    // since this is a simple sample, we have a trivial selection process. We pick
    // the first EGLConfig that matches:
    eglChooseConfig(mEglDisplay, attribs, &mEglConfig, 1, &numConfigs);

    // create EGL surface
    mEglSurface = eglCreateWindowSurface(mEglDisplay, mEglConfig, mApp->window, NULL);
    if (mEglSurface == EGL_NO_SURFACE) {
        LOGE("Failed to create EGL surface, EGL error %d", eglGetError());
        return false;
    }

    LOGD("NativeEngine: successfully initialized surface.");
    return true;
}

bool FGEngine::InitContext() {
    // need a display
    MY_ASSERT(mEglDisplay != EGL_NO_DISPLAY);

    EGLint attribList[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE }; // OpenGL 2.0

    if (mEglContext != EGL_NO_CONTEXT) {
        // nothing to do
        LOGD("NativeEngine: no need to init context (already had one).");
        return true;
    }

    LOGD("NativeEngine: initializing context.");

    // create EGL context
    mEglContext = eglCreateContext(mEglDisplay, mEglConfig, NULL, attribList);
    if (mEglContext == EGL_NO_CONTEXT) {
        LOGE("Failed to create EGL context, EGL error %d", eglGetError());
        return false;
    }

    LOGD("NativeEngine: successfull initialized context.");

    return true;
}

bool FGEngine::HandleEglError(EGLint error) {
    switch (error) {
        case EGL_SUCCESS:
            // nothing to do
            return true;
        case EGL_CONTEXT_LOST:
            LOGW("NativeEngine: egl error: EGL_CONTEXT_LOST. Recreating context.");
            KillContext();
            return true;
        case EGL_BAD_CONTEXT:
            LOGW("NativeEngine: egl error: EGL_BAD_CONTEXT. Recreating context.");
            KillContext();
            return true;
        case EGL_BAD_DISPLAY:
            LOGW("NativeEngine: egl error: EGL_BAD_DISPLAY. Recreating display.");
            KillDisplay();
            return true;
        case EGL_BAD_SURFACE:
            LOGW("NativeEngine: egl error: EGL_BAD_SURFACE. Recreating display.");
            KillSurface();
            return true;
        default:
            LOGW("NativeEngine: unknown egl error: %d", error);
            return false;
    }
}

void FGEngine::KillDisplay() {
    // causes context and surface to go away too, if they are there
    LOGD("NativeEngine: killing display.");
    KillContext();
    KillSurface();

    if (mEglDisplay != EGL_NO_DISPLAY) {
        LOGD("NativeEngine: terminating display now.");
        eglTerminate(mEglDisplay);
        mEglDisplay = EGL_NO_DISPLAY;
    }
    LOGD("NativeEngine: display killed successfully.");
}

void FGEngine::ConfigureOpenGL() {
    glClearColor(1.0f, 0.5f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}
