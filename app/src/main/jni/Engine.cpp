//
// Created by gert on 2020/10/23.
//

#include "Engine.h"

Engine::Engine()
{
    vulkanEngine = new VulkanEngine();

    data.counter = 0;
    data.x = 0;
    data.y = 0;
    data.animating = 1;
}

Engine::~Engine()
{
    delete vulkanEngine;
}
