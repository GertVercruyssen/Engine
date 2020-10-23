//
// Created by gert on 2020/10/23.
//

#ifndef TUTORIAL04_FIRST_WINDOW_ENGINE_H
#define TUTORIAL04_FIRST_WINDOW_ENGINE_H

#include <android_native_app_glue.h>

/**
 * Our saved state data.
 */
struct savedData {
    unsigned int counter;
    int x;
    int y;
    bool animating;
};

class Engine {
public:
    Engine();
    virtual ~Engine();
    android_app* state; //state received from Android
    savedData data; //the game data kept between switching apps
private:

};


#endif //TUTORIAL04_FIRST_WINDOW_ENGINE_H
