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

        enum Options {
            NO_RELOAD = 1 << 0,
            NO_RELOAD_ONCE = 1 << 1,
        };

        AssetManager();
        void addSearchDirectory(const std::string &directory);
        void removeSearchDirectory(const std::string &directory);
        std::string searchFile(const std::string &file);
        std::string minimalFilePath(const std::string &file);
        const std::vector<std::string> &getSearchDirectories(){
            return searchDirectories;
        }

        //get an asset by file name
        //the asset is loaded asynchronous if not specified otherwise
        template<typename T>
        Ref<T> get(const std::string &file, bool synchronous = false,
            const std::function<bool(Ref<Asset>)> &preLoad = nullptr, const std::function<bool(Ref<Asset>)> &postLoad = nullptr){
            return std::static_pointer_cast<T>(get(env->reflection->getTypeId<T>(), file, synchronous, preLoad, postLoad));
        }

        //get an asset by file name
        //the asset is loaded asynchronous if not specified otherwise
        Ref<Asset> get(int typeId, const std::string &file, bool synchronous = false,
            const std::function<bool(Ref<Asset>)> &preLoad = nullptr, const std::function<bool(Ref<Asset>)> &postLoad = nullptr);

        template<typename T>
        std::string getFile(Ref<T> asset){
            return getFile(std::static_pointer_cast<Asset>(asset));
        }

        std::string getFile(Ref<Asset> asset);
        Status getStatus(const std::string &file);
        void setOptions(const std::string &file, Options options);
        void unload(const std::string &file);
        std::vector<std::string> getAssetList(int typeId = -1);

        //unload all assets that are not in use
        //if they get used again, they get loaded again
        void unloadAllUnused();
        bool isLoadingInProcess(int typeId = -1);
        bool isUsed(const std::string& file);

        void startup() override;
        void update() override;
        void shutdown() override;

    private:
        class AssetRecord{
        public:
            Ref<Asset> asset;
            std::string file;
            std::string path;
            Status status;
            Options options;
            int typeId;
            uint64_t timeStamp;
            uint64_t previousTimeStamp;
            std::function<bool(Ref<Asset>)> preLoad;
            std::function<bool(Ref<Asset>)> postLoad;
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

