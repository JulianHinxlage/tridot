//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "AssetManager.h"
#include "core/ThreadPool.h"
#include "engine/Time.h"

namespace tri {






    TRI_REGISTER_SYSTEM_INSTANCE(AssetManager, env->assets);

    uint64_t getTimeStamp(const std::string &file){
        if(!std::filesystem::exists(file)){
            return 0;
        }else{
            return std::filesystem::last_write_time(file).time_since_epoch().count();
        }
    }

    AssetManager::AssetManager() {
        hotReloadEnabled = false;
        asynchronousEnabled = true;
        running = false;
    }

    void AssetManager::addSearchDirectory(const std::string &directory) {
        if(directory.size() > 0 && directory.back() == '/'){
            searchDirectories.push_back(directory);
        }else{
            searchDirectories.push_back(directory + "/");
        }
    }

    void AssetManager::removeSearchDirectory(const std::string &directory) {
        for(int i = 0; i < searchDirectories.size(); i++){
            auto &dir = searchDirectories[i];
                if(dir == directory || dir == directory + "/"){
                searchDirectories.erase(searchDirectories.begin() + i);
                i--;
            }
        }
    }

    std::string AssetManager::searchFile(const std::string &file) {
        for(auto &dir : searchDirectories){
            if(std::filesystem::exists(dir + file)){
                return dir + file;
            }
        }
        return "";
    }

    Ref<Asset> AssetManager::get(int typeId, const std::string &file, bool synchronous,
        const std::function<bool()> &preLoad, const std::function<bool()> &postLoad) {
        auto x = assets.find(file);
        if(x != assets.end()){
            if(x->second.typeId == typeId){
                return x->second.asset;
            }else{
                return nullptr;
            }
        }

        //create asset instance
        auto *desc = env->reflection->getType(typeId);
        Ref<Asset> asset = std::shared_ptr<Asset>((Asset*)desc->alloc(), [desc](Asset *ptr){ desc->free(ptr);});

        AssetRecord &record = assets[file];
        record.asset = asset;
        record.typeId = typeId;
        record.file = file;
        record.status = UNLOADED;
        record.fullPath = "";
        record.timeStamp = 0;
        record.previousTimeStamp = 0;
        record.preLoad = preLoad;
        record.postLoad = postLoad;

        if(synchronous || !asynchronousEnabled){
            load(record);
            loadActivate(record);
        }else{
            wakeCondition.notify_one();
        }

        return record.asset;
    }

    AssetManager::Status AssetManager::getStatus(Ref<Asset> asset) {
        for (auto &iter : assets) {
            auto &record = iter.second;
            if(record.asset == asset){
                return record.status;
            }
        }
        return UNLOADED;
    }

    void AssetManager::unload(Ref<Asset> asset) {
        for (auto &iter : assets) {
            auto &record = iter.second;
            if(record.asset == asset){
                record.status = (Status) (record.status | SHOULD_NOT_LOAD);
            }
        }
    }

    void AssetManager::load(Ref<Asset> asset) {
        for (auto &iter : assets) {
            auto &record = iter.second;
            if(record.asset == asset){
                record.status = (Status) (record.status & ~(int)SHOULD_NOT_LOAD);
            }
        }
    }

    void AssetManager::unloadAllUnused() {
        bool change = true;
        while(change) {
            change = false;
            for (auto &iter : assets) {
                auto &record = iter.second;
                if (record.status & LOADED) {
                    if (record.asset.use_count() <= 1) {
                        assets.erase(iter.first);
                        change = true;
                        break;
                    }
                }
            }
        }
    }

    void AssetManager::startup() {
        running = true;
        for(int i = 0; i < 16; i++){
            threadIds.push_back(env->threads->addThread([&](){
                bool processed = false;
                while(running){
                    if(!processed){
                        std::unique_lock<std::mutex> lock(mutex);
                        wakeCondition.wait(lock);
                    }

                    if(!running){
                        break;
                    }

                    processed = false;
                    for(auto &iter : assets) {
                        auto &record = iter.second;
                        if(load(record)){
                            processed = true;
                            break;
                        }
                    }
                }
            }));
        }
    }

    void AssetManager::update() {
        Clock clock;
        for(auto &iter : assets) {
            auto &record = iter.second;
            if(loadActivate(record)){
                if(clock.elapsed() > 0.010){
                    break;
                }
            }
        }

        if(hotReloadEnabled){
            if(env->time->frameTicks(1.0)) {
                for (auto &iter : assets) {
                    auto &record = iter.second;
                    if((record.status & LOADED) && !(record.status & FAILED_TO_LOAD)) {
                        if(!(record.status & SHOULD_NOT_LOAD)) {
                            if (record.timeStamp != 0) {
                                if (record.fullPath != "") {
                                    uint64_t currentTimeStamp = getTimeStamp(record.fullPath);
                                    if (currentTimeStamp != 0) {
                                        if (currentTimeStamp != record.timeStamp) {
                                            record.status = UNLOADED;
                                            if (!asynchronousEnabled) {
                                                load(record);
                                                loadActivate(record);
                                            } else {
                                                wakeCondition.notify_one();
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    void AssetManager::shutdown() {
        running = false;
        wakeCondition.notify_all();
        for(int threadId : threadIds){
            env->threads->jointThread(threadId);
            env->threads->terminateThread(threadId);
        }
        threadIds.clear();
        assets.clear();
    }

    bool AssetManager::load(AssetManager::AssetRecord &record) {
        bool processed = false;
        if (record.status & UNLOADED) {
            if (!(record.status & FAILED_TO_LOAD)) {
                if(!(record.status & SHOULD_NOT_LOAD)) {

                    if (!(record.status & STATE_PRE_LOADED)) {
                        if (record.preLoad == nullptr || record.preLoad()) {
                            record.status = (Status) (record.status | STATE_PRE_LOADED);
                        } else {
                            record.status = (Status) (record.status | FAILED_TO_LOAD);
                            env->console->warning("failed to load asset: ", record.file);
                        }
                        processed = true;
                    }

                    if ((record.status & STATE_PRE_LOADED) && !(record.status & STATE_LOADED)) {
                        record.fullPath = searchFile(record.file);
                        if (record.fullPath == "") {
                            record.status = (Status) (record.status | FAILED_TO_LOAD);
                            record.status = (Status) (record.status | FILE_NOT_FOUND);
                            env->console->warning("asset file not found: ", record.file);
                        } else {
                            record.previousTimeStamp = record.timeStamp;
                            record.timeStamp = getTimeStamp(record.fullPath);
                            if (record.asset->load(record.fullPath)) {
                                record.status = (Status) (record.status | STATE_LOADED);
                            } else {
                                record.status = (Status) (record.status | FAILED_TO_LOAD);
                                env->console->warning("failed to load asset: ", record.file);
                            }
                        }
                        processed = true;
                    }

                }
            }
        }
        return processed;
    }

    bool AssetManager::loadActivate(AssetManager::AssetRecord &record) {
        bool processed = false;
        if (record.status & UNLOADED) {
            if (!(record.status & FAILED_TO_LOAD)) {
                if(!(record.status & SHOULD_NOT_LOAD)) {

                    if ((record.status & STATE_LOADED) && !(record.status & STATE_ACTIVATED)) {
                        if (record.asset->loadActivate()) {
                            record.status = (Status) (record.status | STATE_ACTIVATED);
                        } else {
                            record.status = (Status) (record.status | FAILED_TO_LOAD);
                            env->console->warning("failed to load asset: ", record.file);
                        }
                        processed = true;
                    }

                    if ((record.status & STATE_ACTIVATED) && !(record.status & STATE_POST_LOADED)) {
                        if (record.postLoad == nullptr || record.postLoad()) {
                            record.status = (Status) (record.status | STATE_POST_LOADED);
                            record.status = (Status) (record.status | LOADED);
                            record.status = (Status) (record.status & ~(int) UNLOADED);
                            if(record.previousTimeStamp != 0){
                                env->console->debug("reloaded asset: ", record.file);
                            }else{
                                env->console->debug("loaded asset: ", record.file);
                            }
                        } else {
                            record.status = (Status) (record.status | FAILED_TO_LOAD);
                            env->console->warning("failed to load asset: ", record.file);
                        }
                        processed = true;
                    }

                }
            }
        }
        return processed;
    }

}
