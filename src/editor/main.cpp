//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "core/core.h"
#include "render/Window.h"
#include "engine/AssetManager.h"

using namespace tri;

int main(int argc, char* argv[]) {
    Environment::startup();

    env->profiler->beginPhase("startup");
    env->signals->preStartup.invoke();

    env->assets->hotReloadEnabled = true;
    env->console->setVariable<bool>("hot_reloading", &env->assets->hotReloadEnabled);
    env->console->setVariable<int>("resolution_x", 1920);
    env->console->setVariable<int>("resolution_y", 1080);
    env->console->setVariable<bool>("vsync", true);

    std::string configFile = "../res/config.txt";
    if (argc > 1) {
        configFile = argv[1];
    }
    env->console->loadConfigFile(configFile);

    env->window->init(
        *env->console->getVariable<int>("resolution_x"),
        *env->console->getVariable<int>("resolution_y"),
        "Tridot Editor");
    env->window->setBackgroundColor(Color(50, 50, 50));

    env->signals->startup.invoke(true);
    env->signals->postStartup.invoke();
    env->profiler->endPhase();

    while (env->window->isOpen()) {
        env->profiler->beginPhase("update");
        env->signals->preUpdate.invoke();
        env->signals->update.invoke(true);
        env->signals->postUpdate.invoke();

        env->window->setVSync(*env->console->getVariable<bool>("vsync"));
        env->profiler->endPhase();
        env->profiler->nextFrame();
    }

    env->profiler->beginPhase("shutdown");
    env->signals->preShutdown.invoke();
    env->signals->shutdown.invoke();
    env->signals->postShutdown.invoke();
    env->profiler->endPhase();

    Environment::shutdown();
    return 0;
}
