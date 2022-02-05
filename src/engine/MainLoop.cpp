//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "MainLoop.h"
#include "core/core.h"
#include "engine/AssetManager.h"
#include "render/Window.h"
#include "render/RenderThread.h"
#include "render/RenderContext.h"

namespace tri {

    void MainLoop::startup(const std::vector<std::string>& configFileList) {
        Environment::startup();
        TRI_PROFILE_PHASE("startup");

        env->console->setVariable<bool>("hot_reloading", &env->assets->hotReloadEnabled);
        env->console->setVariable<int>("resolution_x", 1920);
        env->console->setVariable<int>("resolution_y", 1080);
        env->console->setVariable<int>("vsync", 1);
        env->console->setVariable<std::string>("log_file", "log.txt");

        {
            TRI_PROFILE("preStartup");
            env->signals->preStartup.invoke();
        }
        {
            TRI_PROFILE("loadConfigFile")
            for (auto& file : configFileList) {
                if (!file.empty()) {
                    if (std::filesystem::exists(file)) {
                        env->console->loadConfigFile(file);
                        break;
                    }
                }
            }
        }

        env->console->addLogFile(*env->console->getVariable<std::string>("log_file"), Console::Options(TRACE, true, true, false));

        {
            TRI_PROFILE("launchRenderThread")
            env->renderThread->useDedicatedThread = false;
            env->renderThread->launch([]() {
                env->window->init(
                        *env->console->getVariable<int>("resolution_x"),
                        *env->console->getVariable<int>("resolution_y"),
                        "Tridot Editor");
                env->window->setBackgroundColor(Color(50, 50, 50));

                {
                    TRI_PROFILE("startup");
                    env->signals->startup.invokeProfile();
                }
                {
                    TRI_PROFILE("postStartup");
                    env->signals->postStartup.invoke();
                }
            });
        }

    }

    void MainLoop::run() {
        while (env->window->isOpen()) {
            TRI_PROFILE_PHASE("update");
            {
                TRI_PROFILE("preUpdate");
                env->signals->preUpdate.invoke();
            }
            {
                TRI_PROFILE("update");
                env->signals->update.invokeProfile();
            }
            {
                TRI_PROFILE("postUpdate");
                env->signals->postUpdate.invoke();
            }
            env->window->setVSync(*env->console->getVariable<int>("vsync"));
            TRI_PROFILE_FRAME;
        }
    }

    void MainLoop::shutdown() {
        {
            TRI_PROFILE_PHASE("shutdown");
            env->renderThread->addTask([]() {
                {
                    TRI_PROFILE("preShutdown");
                    env->signals->preShutdown.invoke();
                }
                {
                    TRI_PROFILE("shutdown");
                    env->signals->shutdown.invokeProfile();
                }
                {
                    TRI_PROFILE("postShutdown");
                    env->signals->postShutdown.invoke();
                }
            });
            env->renderThread->synchronize();
            env->renderThread->terminate();
        }
        Environment::shutdown();
    }

}
