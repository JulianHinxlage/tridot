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
        env->console->setVariable<bool>("use_render_thread", &env->renderThread->useDedicatedThread);

        {
            TRI_PROFILE("preStartup");
            env->signals->preStartup.invokeProfile();
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
            //performe startup on render thread due to render context issues
            TRI_PROFILE("launchRenderThread")
            env->renderThread->launch([]() {
                env->window->init(
                        *env->console->getVariable<int>("resolution_x"),
                        *env->console->getVariable<int>("resolution_y"),
                        "Tridot Editor");
                env->window->setBackgroundColor(Color(50, 50, 50));

                {
                    TRI_PROFILE("startup");
                    env->signals->startup.invokeProfile();
                    env->signals->startup.setActiveAll(false);
                }
                {
                    TRI_PROFILE("postStartup");
                    env->signals->postStartup.invokeProfile();
                }
            });
        }

    }

    void MainLoop::run() {
        while (env->window->isOpen()) {
            TRI_PROFILE_PHASE("update");

            //invoke new startup signal callbacks
            env->signals->startup.invoke();
            env->signals->startup.setActiveAll(false);

            {
                TRI_PROFILE("preUpdate");
                env->signals->preUpdate.invokeProfile();
            }
            {
                TRI_PROFILE("update");
                env->signals->update.invokeProfile();
            }
            {
                TRI_PROFILE("postUpdate");
                env->signals->postUpdate.invokeProfile();
            }
            env->window->setVSync(*env->console->getVariable<int>("vsync"));
            TRI_PROFILE_FRAME;
        }
    }

    void MainLoop::shutdown() {
        {
            TRI_PROFILE_PHASE("shutdown");
            //performe shutdown on render thread due to render context issues
            env->renderThread->addTask([]() {
                {
                    TRI_PROFILE("preShutdown");
                    env->signals->preShutdown.invokeProfile();
                }
                {
                    TRI_PROFILE("shutdown");
                    env->signals->shutdown.invokeProfile();
                    env->profiler = nullptr;
                }
                {
                    TRI_PROFILE("postShutdown");
                    env->signals->postShutdown.invokeProfile();
                }
            });
            env->renderThread->synchronize();
            env->renderThread->terminate();

#if TRACY_ENABLE
            //close tracy connection
            if (TracyIsConnected) {
                tracy::GetProfiler().RequestShutdown();
                while (!tracy::GetProfiler().HasShutdownFinished()) {}
            }
#endif
        }
        Environment::shutdown();
    }

}
