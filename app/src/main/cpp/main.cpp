/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

//BEGIN_INCLUDE(all)
#include <initializer_list>
#include <memory>
#include <cstdlib>
#include <cstring>
#include <jni.h>
#include <cerrno>
#include <cassert>

#include <vulkan/vulkan.h>

//#include <EGL/egl.h>
//#include <GLES/gl.h>

#include <android/log.h>
#include <android_native_app_glue.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

/**
 * Our saved state data.
 */
struct saved_state {
    uint32_t counter;
    int32_t x;
    int32_t y;
};

/**
 * Shared state for our app.
 */
struct engine {
    struct android_app* app;
    //display
    int animating;
    int32_t width;
    int32_t height;
    struct saved_state state;
};

/**
 * Initialize engine
 */
static int engine_init(struct engine* engine) {
//    engine->display = display;
//    engine->context = context;
//    engine->surface = surface;
//    engine->width = w;
//    engine->height = h;

    LOGI("intialized");
    return 0;
}

/**
 * Draw frame
 */
static void engine_draw(struct engine* engine) {

}

/**
 * Destroy engine
 */
static void engine_destroy(struct engine* engine) {

}

/**
 * Process the next input event.
 */
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    auto* engine = (struct engine*)app->userData;
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        engine->animating = 1;
        engine->state.counter++;
        engine->state.x = AMotionEvent_getX(event, 0);
        engine->state.y = AMotionEvent_getY(event, 0);
        return 1;
    }
    return 0;
}

/**
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    auto* engine = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            // The system has asked us to save our current state.  Do so.
            engine->app->savedState = malloc(sizeof(struct saved_state));
            *((struct saved_state*)engine->app->savedState) = engine->state;
            engine->app->savedStateSize = sizeof(struct saved_state);
            break;
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it ready.
            if (engine->app->window != nullptr) {
                engine_init(engine);
                engine_draw(engine);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            engine_destroy(engine);
            break;
        case APP_CMD_GAINED_FOCUS:
            // When our app gains focus, we start drawing
            engine->animating = 1;
            break;
        case APP_CMD_LOST_FOCUS:
            // When our app loses focus, we stop animating.
            engine->animating = 0;
            engine_draw(engine);
            break;
        default:
            break;
    }
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state) {
    struct engine engine{};

    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;

    if (state->savedState != nullptr) {
        // We are starting with a previous saved state; restore from it.
        engine.state = *(struct saved_state*)state->savedState;
    } else {
        engine.state.x = 0;
        engine.state.y = 0;
        engine.state.counter = 0;
    }

    // loop waiting for stuff to do.

    while (true)
    {
        // Read all pending events.
        int events;
        struct android_poll_source* source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((ALooper_pollAll(engine.animating ? 0 : -1, nullptr, &events, (void **) &source)) >= 0)
        {
            // Process this event.
            if (source != nullptr)
            {
                source->process(state, source);
            }

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                engine_destroy(&engine);
                return;
            }
        }

        if (engine.animating)
        {
            // Drawing is throttled to the screen update rate, so there
            // is no need to do timing here.
            engine_draw(&engine);
        }
    }
}
//END_INCLUDE(all)
