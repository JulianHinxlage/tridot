//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once
#include "core/core.h"
#include "core/util/Ref.h"
#include "core/System.h"
#include "Asset.h"
#include <atomic>

namespace tri {

    class AssetManager : public System {
    public:
        bool hotReloadEnabled;
        bool asynchronousEnabled;

        enum Status {
            UNLOADED = 1,
            LOADED = 2,
            FAILED_TO_LOAD = 4,
            FILE_NOT_FOUND = 8,
            STATE_PRE_LOADED = 16,
            STATE_LOADED = 32,
            STATE_ACTIVATED = 64,
            STATE_POST_LOADED = 128,
            SHOULD_NOT_LOAD = 256,
        };

        AssetManager();
        void addSearchDirectory(const std::string &directory);
        void removeSearchDirectory(const std::string &directory);
        std::string searchFile(const std::string &file);

        //get an asset by file name
        //the asset is loaded asynchronous if not specified otherwise
        template<typename T>
        Ref<T> get(const std::string &file, bool synchronous = false,
            const std::function<bool()> &preLoad = nullptr, const std::function<bool()> &postLoad = nullptr){
            return std::static_pointer_cast<T>(get(env->reflection->getTypeId<T>(), file, synchronous, preLoad, postLoad));
        }

        //get an asset by file name
        //the asset is loaded asynchronous if not specified otherwise
        Ref<Asset> get(int typeId, const std::string &file, bool synchronous = false,
            const std::function<bool()> &preLoad = nullptr, const std::function<bool()> &postLoad = nullptr);


        Status getStatus(Ref<Asset> asset);

        //mark an loaded asset to be unloaded
        void unload(Ref<Asset> asset);

        //make an unloaded asset to be loaded
        //for normal loading use the get function
        void load(Ref<Asset> asset);

        //unload all assets that are not in use
        //if they get used again, they get loaded again
        void unloadAllUnused();

        void startup() override;
        void update() override;
        void shutdown() override;

    private:
        class AssetRecord{
        public:
            Ref<Asset> asset;
            std::string file;
            std::string fullPath;
            Status status;
            int typeId;
            uint64_t timeStamp;
            uint64_t previousTimeStamp;
            std::function<bool()> preLoad;
            std::function<bool()> postLoad;
            std::atomic_bool locked;
        };

        std::vector<std::string> searchDirectories;
        std::unordered_map<std::string, AssetRecord> assets;

        std::vector<int> threadIds;
        bool running;
        std::mutex mutex;
        std::condition_variable wakeCondition;

        bool load(AssetRecord &record);
        bool loadActivate(AssetRecord &record);
    };

}

