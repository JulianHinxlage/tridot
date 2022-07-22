//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Map.h"
#include "AssetManager.h"
#include "Serializer.h"
#include "ComponentCache.h"

namespace tri {

    void Map::setToActiveWorld() {
        if (world) {
            env->eventManager->postTick.addListener([&]() {
                env->world->copy(*world);
                env->worldFile = file;
                env->systemManager->getSystem<ComponentCache>()->copyWorld(world.get(), env->world);
                env->eventManager->onMapBegin.invoke(env->world, file);
            }, true);
        }
    }

    void Map::loadAndSetToActiveWorld(const std::string &file) {
        auto map = env->assetManager->get<Map>(file, AssetManager::NONE, nullptr, [](auto asset) {
            ((Map*)asset.get())->setToActiveWorld();
            env->eventManager->postTick.addListener([file = env->assetManager->getFile(asset)]() {
                env->assetManager->unload(file);
            }, true);
            return true;
        });
        if (env->assetManager->getStatus(file) & AssetManager::Status::LOADED) {
            map->setToActiveWorld();
            env->eventManager->postTick.addListener([file]() {
                env->assetManager->unload(file);
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
        env->serializer->serializeWorld(world.get(), file);
        return true;
    }

}

