//
// Created by parallelno on 02.04.2016.
//

#ifndef ENGINE_FGENGINE_H
#define ENGINE_FGENGINE_H

#include "common.hpp"

struct NativeEngineSavedState{
};

class FGEngine {
public:
    FGEngine(struct android_app *app);
    ~FGEngine();

    void GameLoop();
private:

    struct android_app *mApp;
    bool mHasFocus;
    bool mIsVisible;
    bool mHasWindow;
    bool mHasGLObjects;
    EGLDisplay mEglDisplay;
    EGLSurface mEglSurface;
    EGLContext mEglContext;
    EGLConfig mEglConfig;
    int mSurfWidth;
    int mSurfHeight;
    int mApiVersion;
    JNIEnv *mJniEnv;
    struct NativeEngineSavedState mState;
    bool mIsFirstFrame;

    void KillContext();
    void KillSurface();
    void KillDisplay();

    bool IsAnimating();

    void DoFrame();
    bool PrepareToRender();
    void ConfigureOpenGL();
    bool InitDisplay();
    bool InitSurface();
    bool InitContext();

    bool HandleEglError(EGLint error);
public:
    // these are public for simplicity because we have internal static callbacks
    void HandleCommand(int32_t cmd);
    bool HandleInput(AInputEvent *event);
};


#endif //ENGINE_FGENGINE_H
