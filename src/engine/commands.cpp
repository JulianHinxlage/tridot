//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "core/core.h"
#include "render/Window.h"
#include "engine/Serializer.h"
#include "engine/AssetManager.h"

namespace tri{


    TRI_REGISTER_CALLBACK() {
        env->console->addCommand("module_load", [](const std::vector<std::string>& args) {
            if (args.size() > 1) {
                env->modules->loadModule(args[1]);
            }
            else {
                env->console->info("usage: module_load <module>");
            }
        }, true);
        env->console->addCommand("module_unload", [](const std::vector<std::string>& args) {
            if (args.size() > 1) {
                env->modules->unloadModule(env->modules->getModule(args[1]));
            }
            else {
                env->console->info("usage: module_unload <module>");
            }
        }, true);

        env->console->addCommand("exit", []() {
            env->window->close();
        });
        env->console->addCommand("quit", []() {
            env->window->close();
        });

        env->console->addCommand("print_types", []() {
            for (auto& desc : env->reflection->getDescriptors()) {
                if (desc) {
                    env->console->info("type ", desc->name, ": size = ", desc->size, ", typeId = ", desc->typeId);
                    for (auto& m : desc->member) {
                        env->console->info(" member ", m.name, ": type = ", m.type->name, ", offset = ", m.offset);
                    }
                    for (auto& c : desc->constants) {
                        env->console->info(" constant: ", c.name, " = ", c.value);
                    }
                }
            }
        });
        env->console->addCommand("print_callbacks", [](const std::vector<std::string>& args) {
            if (args.size() > 1) {
                for (auto& o : env->signals->getSignal<>(args[1]).getObservers()) {
                    env->console->info("callback: ", o.name);
                }
            }
            else {
                env->console->info("usage: print_callbacks <signal>");
            }
        });

        env->console->addCommand("signal_activate", [](const std::vector<std::string>& args) {
            if (args.size() >= 3) {
                std::vector<std::string> callbacks(args.begin() + 2, args.end());
                env->signals->getSignal<>(args[1]).setActiveCallbacks(callbacks, true);
            }
            else {
                env->console->info("usage: signal_activate <signal> <callbacks>");
            }
        });
        env->console->addCommand("signal_deactivate", [](const std::vector<std::string>& args) {
            if (args.size() >= 3) {
                std::vector<std::string> callbacks(args.begin() + 2, args.end());
                env->signals->getSignal<>(args[1]).setActiveCallbacks(callbacks, false);
            }
            else {
                env->console->info("usage: signal_deactivate <signal> <callbacks>");
            }
        });
        env->console->addCommand("signal_order", [](const std::vector<std::string>& args) {
            if (args.size() >= 3) {
                std::vector<std::string> callbacks(args.begin() + 2, args.end());
                env->signals->getSignal<>(args[1]).callbackOrder(callbacks);
            }
            else {
                env->console->info("usage: signal_order <signal> <callbacks>");
            }
        });

        env->console->addCommand("scene_load", [](const std::vector<std::string>& args) {
            if (args.size() > 1) {
                std::string fullPath = env->assets->searchFile(args[1]);
                if(fullPath != ""){
                    Clock clock;
                    env->serializer->deserializeScene(fullPath, *env->scene);
                    env->scene->update();
                    env->signals->sceneLoad.invoke(env->scene);
                    env->console->info("loaded scene ", args[1], " in ", clock.elapsed(), "s");
                }else{
                    env->console->warning("scene file not found: ", args[1]);
                }
            }
            else {
                env->console->info("usage: scene_load <scene>");
            }
        }, true);
        env->console->addCommand("scene_save", [](const std::vector<std::string>& args) {
            if (args.size() > 1) {
                std::string fullPath = env->assets->searchFile(args[1]);
                if(fullPath == ""){
                    fullPath = args[1];
                }
                env->serializer->serializeScene(fullPath, *env->scene);
            }
            else {
                env->console->info("usage: scene_save <scene>");
            }
        });

        env->console->addCommand("asset_directory", [](const std::vector<std::string>& args) {
            if (args.size() > 1) {
                env->assets->addSearchDirectory(args[1]);
            }
            else {
                env->console->info("usage: asset_directory <directory>");
            }
        });

        env->console->addCommand("asset_unload_unused", [](const std::vector<std::string>& args) {
            env->assets->unloadAllUnused();
        });
    }

    TRI_REGISTER_CALLBACK() {
        env->signals->preStartup.addCallback("version", []() {
#if TRI_WINDOWS
            env->console->options.color = false;
#endif
            env->console->info("Tridot version ", TRI_VERSION);
        });
    }

}
