//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "core/core.h"
#include "render/Window.h"

using namespace tri;

int main(int argc, char* argv[]) {
    Environment::startup();

    env->signals->preStartup.invoke();

    env->console->setVariable<bool>("vsync", true);
    env->window->init(1080, 720, "Tridot Editor");
    env->window->setBackgroundColor(Color(50, 50, 50));

    env->signals->startup.invoke();

    std::string configFile = "../res/config.txt";
    if (argc > 1) {
        configFile = argv[1];
    }
    env->console->loadConfigFile(configFile);

    env->signals->postStartup.invoke();

    while (env->window->isOpen()) {
        env->signals->preUpdate.invoke();
        env->signals->update.invoke();
        env->signals->postUpdate.invoke();

        env->window->setVSync(*env->console->getVariable<bool>("vsync"));
    }

    env->signals->preShutdown.invoke();
    env->signals->shutdown.invoke();
    env->signals->postShutdown.invoke();

    Environment::shutdown();
    return 0;
}
