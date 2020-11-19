//
// Created by gert on 2020/10/23.
//

#include "Engine.h"

Engine::Engine()
{
    vulkanEngine = new VulkanEngine();

    data.counter = 0;
    data.x = 0.0f;
    data.y = 0.0f;
    data.animating = 1;
}

Engine::~Engine()
{
    delete vulkanEngine;
}
