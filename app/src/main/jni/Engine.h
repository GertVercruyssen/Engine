//
// Created by gert on 2020/10/23.
//

#ifndef TUTORIAL04_FIRST_WINDOW_ENGINE_H
#define TUTORIAL04_FIRST_WINDOW_ENGINE_H

#include <android_native_app_glue.h>
#include "VulkanMain.hpp"

/**
 * Our saved state data.
 */
struct savedData {
    unsigned int counter;
    float x;
    float y;
    bool animating;
};

class Engine {
public:
    Engine();
    virtual ~Engine();
    android_app* state; //state received from Android
    savedData data; //the game data kept between switching apps
    VulkanEngine* vulkanEngine; //The actual graphics code
private:

};


#endif //TUTORIAL04_FIRST_WINDOW_ENGINE_H
