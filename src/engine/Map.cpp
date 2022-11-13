//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Map.h"
#include "AssetManager.h"
#include "engine/Serializer.h"
#include "ComponentCache.h"
#include "RuntimeMode.h"

namespace tri {

    TRI_ASSET(Map);

    void Map::setToActiveWorld() {
        if (world) {
            env->eventManager->postTick.addListener([&]() {
                if (env->editor) {
                    env->runtimeMode->setMode(RuntimeMode::EDIT);
                }

                env->eventManager->onMapEnd.invoke(env->world, file);
                env->world->copy(*world);

                env->worldFile = file;
                env->systemManager->getSystem<ComponentCache>()->copyWorld(world.get(), env->world);
                env->eventManager->onMapBegin.invoke(env->world, file);
            }, true);
        }
    }

    void Map::loadAndSetToActiveWorld(const std::string &file) {
        env->runtimeMode->setMode(RuntimeMode::LOADING);
        auto map = env->assetManager->get<Map>(file, AssetManager::Options::EXPLICIT_LOAD, nullptr, [](auto asset) {
            ((Map*)asset.get())->setToActiveWorld();
            env->eventManager->postTick.addListener([file = env->assetManager->getFile(asset)]() {
                env->assetManager->unload(file);
                if (!env->editor) {
                    env->runtimeMode->setMode(RuntimeMode::PLAY);
                }
            }, true);
            return true;
        });
        if (env->assetManager->getStatus(file) & AssetManager::Status::LOADED) {
            map->setToActiveWorld();
            env->eventManager->postTick.addListener([file]() {
                env->assetManager->unload(file);
                if (!env->editor) {
                    env->runtimeMode->setMode(RuntimeMode::PLAY);
                }
            }, true);
        }
    }

    bool Map::load(const std::string& file) {
        if (!world) {
            world = std::make_shared<World>();
        }
        this->file = file;
        bool result = env->serializer->deserializeWorld(world.get(), file);
        if (result) {
            env->eventManager->onMapLoad.invoke(world.get(), file);
        }
        return result;
    }

    bool Map::save(const std::string& file) {
        env->assetManager->setOptions(file, AssetManager::Options::NO_RELOAD_ONCE);
        if (!world) {
            world = std::make_shared<World>();
        }
        env->serializer->serializeWorld(world.get(), file);
        return true;
    }

}

