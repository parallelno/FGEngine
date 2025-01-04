//
// Created by parallelno on 02.04.2016.
//

#ifndef ENGINE_COMMON_HPP
#define ENGINE_COMMON_HPP

extern "C" {
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <EGL/eglext.h>
#include <jni.h>
#include <errno.h>
#include <android/sensor.h>
#include <android/log.h>
#include "../native_app_glue/android_native_app_glue.h"
#include <unistd.h>
}

#define DEBUG_TAG "EndlessTunnel:Native"
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG, __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, DEBUG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, DEBUG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, DEBUG_TAG, __VA_ARGS__))
#define ABORT_GAME { LOGE("*** GAME ABORTING."); *((char*)0) = 'a'; }
#define DEBUG_BLIP LOGD("[ BLIP ]: %s:%d", __FILE__, __LINE__)

#define MY_ASSERT(cond) { if (!(cond)) { LOGE("ASSERTION FAILED: %s", #cond); \
   ABORT_GAME; } }

#define BUFFER_OFFSET(i) ((char*)NULL + (i))


class LogFunc {
public:
    LogFunc(const char *_name) {
        name_ = _name;
        LOGD("====>%s", name_);
    }
    ~LogFunc() {
        LOGD("<====%s", name_);
    }
private:
    const char* name_;
};

#endif //ENGINE_COMMON_HPP
