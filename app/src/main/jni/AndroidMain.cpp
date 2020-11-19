// Copyright 2016 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <android/log.h>
#include <android_native_app_glue.h>
#include "VulkanMain.hpp"
#include "Engine.h"
#include <memory>

// Process the next main command.
void handle_cmd(android_app* app, int32_t cmd) {
    auto* temp_engine = (class Engine*)app->userData;
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            // The system has asked us to save our current state.  Do so.
            temp_engine->state->savedState = malloc(sizeof( struct savedData));
            *((struct savedData*)temp_engine->state->savedState) = temp_engine->data;
            temp_engine->state->savedStateSize = sizeof(struct savedData);
            break;
        case APP_CMD_INIT_WINDOW:
        {;
            // The window is being shown, get it ready.
            temp_engine->vulkanEngine->InitVulkan(app);
            break;
        }
        case APP_CMD_TERM_WINDOW:
        {
            // The window is being hidden or closed, clean it up.
            //Wait to finish current draw order
            temp_engine->vulkanEngine->WaitIdle();
            temp_engine->vulkanEngine->DeleteVulkan();
            break;
        }
        case APP_CMD_GAINED_FOCUS:
            // When our app gains focus, we start drawing
            temp_engine->data.animating = true;
            break;
        case APP_CMD_LOST_FOCUS:
            // When our app loses focus, we stop animating.
            temp_engine->data.animating = false;
            break;
        case APP_CMD_CONTENT_RECT_CHANGED: //TODO: check if this actually gets called on rotation
            //when our screen gets rotated, recreate the swapchain in vulkan
            temp_engine->vulkanEngine->VulkanResize();
        default:
            __android_log_print(ANDROID_LOG_INFO, "Vulkan Tutorials",
                                "event not handled: %d", cmd);
    }
}

/**
 * Process the next input (touch) event.
 */
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    auto* temp_engine = (class Engine*)app->userData;
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        temp_engine->data.animating = true;
        temp_engine->data.counter++;
        temp_engine->data.x = AMotionEvent_getX(event, 0);
        temp_engine->data.y = AMotionEvent_getY(event, 0);
        return 1;
    }
    return 0;
}

void android_main(struct android_app* state) {
    Engine* engine = new Engine();

    if (state->savedState != nullptr) {
        // We are starting with a previous saved state; restore from it.
        engine->data = (*(struct savedData*)state->savedState);
    }

    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onInputEvent = engine_handle_input;

    engine->state = state;
    // Set the callback to process system events
    state->onAppCmd = handle_cmd;

    // Used to poll the events in the main loop
    int events;
    android_poll_source* source;

    // Main loop
    do {
        if (ALooper_pollAll(engine->vulkanEngine->IsVulkanReady() ? 1 : 0, nullptr, &events, (void**)&source) >= 0) {
            if (source != NULL) source->process(state, source);
        }

        // render if vulkan is ready and we be drawing
        if (engine->vulkanEngine->IsVulkanReady() && engine->data.animating) {
            engine->vulkanEngine->VulkanDrawFrame();
        }
    } while (state->destroyRequested == 0);

    delete engine;
}
