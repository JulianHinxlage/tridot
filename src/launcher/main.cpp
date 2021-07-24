//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/core/Environment.h"
#include "tridot/engine/engine.h"
#include "tridot/render/Window.h"

using namespace tridot;

int main(int argc, char *argv[]) {
    //set logging options
    env->console->options.level = DEBUG;
    env->console->options.date = false;
    env->console->addLogFile("log.txt", Console::Options(TRACE, true, true, false));
    env->console->addLogFile("error.txt", Console::Options(ERROR, true, true, false));
#if WIN32
    env->console->options.color = false;
#endif

    //init
    env->events->init.invoke();
    env->events->init.setActiveAll(false);
    env->console->info("Tridot version: ", TRI_VERSION);

    bool running = true;
    env->events->exit.addCallback([&running](){
        running = false;
    });

    //load scene
    env->resources->setup<Scene>("scenes/scene.yml").setInstance(env->scene).get();

    //wait for scene to load but still update window
    while(env->resources->isLoading()){
        env->window->update();
        env->resources->update();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    //main loop
    while(running){
        env->events->init.invoke();
        env->events->init.setActiveAll(false);
        env->events->update.invoke();
        env->events->pollEvents();
    }

    //shutdown
    env->events->shutdown.invoke();
}
