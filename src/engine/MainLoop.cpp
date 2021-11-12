//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "MainLoop.h"
#include "core/core.h"
#include "engine/AssetManager.h"
#include "render/Window.h"

namespace tri {

    void MainLoop::startup(const std::string &configFile, const std::string& fallbackConfigFile) {
        Environment::startup();
        TRI_PROFILE_PHASE("startup");

        env->console->setVariable<bool>("hot_reloading", &env->assets->hotReloadEnabled);
        env->console->setVariable<int>("resolution_x", 1920);
        env->console->setVariable<int>("resolution_y", 1080);
        env->console->setVariable<bool>("vsync", true);

        env->signals->preStartup.invoke();
        if (std::filesystem::exists(configFile)) {
            env->console->loadConfigFile(configFile);
        }
        else if(!fallbackConfigFile.empty()) {
            if (std::filesystem::exists(fallbackConfigFile)) {
                env->console->loadConfigFile(fallbackConfigFile);
            }
        }

        env->window->init(
                *env->console->getVariable<int>("resolution_x"),
                *env->console->getVariable<int>("resolution_y"),
                "Tridot Editor");
        env->window->setBackgroundColor(Color(50, 50, 50));

        env->signals->startup.invoke(true);
        env->signals->postStartup.invoke();
    }

    void MainLoop::run() {
        while (env->window->isOpen()) {
            TRI_PROFILE_PHASE("update");
            env->signals->preUpdate.invoke();
            env->signals->update.invoke(true);
            env->signals->postUpdate.invoke();
            env->window->setVSync(*env->console->getVariable<bool>("vsync"));
            env->profiler->nextFrame();
        }
    }

    void MainLoop::shutdown() {
        {
            TRI_PROFILE_PHASE("shutdown");
            env->signals->preShutdown.invoke();
            env->signals->shutdown.invoke();
            env->signals->postShutdown.invoke();
        }
        Environment::shutdown();
    }

}
