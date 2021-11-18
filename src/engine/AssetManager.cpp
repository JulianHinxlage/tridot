//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "AssetManager.h"
#include "core/ThreadPool.h"
#include "engine/Time.h"
#include "core/util/StrUtil.h"

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
            std::string path = dir + file.substr(StrUtil::match(file, dir));
            if(std::filesystem::exists(path)){
                if(std::filesystem::is_regular_file(path)){
                    return path;
                }
            }
        }
        return "";
    }

    std::string AssetManager::minimalFilePath(const std::string &file) {
        std::string minimalPath = file;
        int maxMatch = 0;
        for(auto &dir : searchDirectories){
            int match = StrUtil::match(file, dir);
            if(match > maxMatch){
                maxMatch = match;
                minimalPath = file.substr(match);
            }
        }
        return minimalPath;
    }

    Ref<Asset> AssetManager::get(int typeId, const std::string &file, bool synchronous,
        const std::function<bool(Ref<Asset>)> &preLoad, const std::function<bool(Ref<Asset>)> &postLoad) {
        std::string minimalPath = minimalFilePath(file);
        auto x = assets.find(minimalPath);
        if(x != assets.end()){
            if(x->second.typeId == typeId){
                return x->second.asset;
            }else{
                return nullptr;
            }
        }

        //create asset instance
        auto *desc = env->reflection->getType(typeId);
        if (!desc) {
            return nullptr;
        }
        Ref<Asset> asset = std::shared_ptr<Asset>((Asset*)desc->alloc(), [desc](Asset *ptr){ desc->free(ptr);});

        AssetRecord &record = assets[minimalPath];
        record.asset = asset;
        record.typeId = typeId;
        record.file = minimalPath;
        record.status = UNLOADED;
        record.path = "";
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

    std::string AssetManager::getFile(Ref<Asset> asset) {
        for (auto &iter : assets) {
            auto &record = iter.second;
            if(record.asset == asset){
                return record.file;
            }
        }
        return "";
    }

    AssetManager::Status AssetManager::getStatus(const std::string &file) {
        auto x = assets.find(minimalFilePath(file));
        if(x != assets.end()){
            return x->second.status;
        }
        return UNLOADED;
    }

    void AssetManager::setOptions(const std::string &file, AssetManager::Options options) {
        auto x = assets.find(minimalFilePath(file));
        if(x != assets.end()){
            x->second.options = (Options)((int)x->second.options | (int)options);
        }
    }

    void AssetManager::unload(const std::string &file) {
        std::string minimalPath = minimalFilePath(file);
        for (auto &iter : assets) {
            auto &record = iter.second;
            if(record.file == minimalPath){
                assets.erase(record.file);
                env->console->debug("unloaded asset: ", minimalPath);
                break;
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
                        std::string file = iter.first;
                        assets.erase(iter.first);
                        env->console->debug("unloaded asset: ", file);
                        change = true;
                        break;
                    }
                }
            }
        }
    }

    bool AssetManager::isUsed(const std::string& file) {
        auto x = assets.find(minimalFilePath(file));
        if (x != assets.end()) {
            return x->second.asset.use_count() > 1;
        }
        return false;
    }

    void AssetManager::startup() {
        running = true;
        for (int i = 0; i < 16; i++) {
            threadIds.push_back(env->threads->addThread([&]() {
                while (running) {
                    bool processed = false;
                    for (auto &iter : assets) {
                        auto &record = iter.second;
                        if(!record.locked.exchange(true)) {
                            if (load(record)) {
                                processed = true;
                                record.locked.store(false);
                                break;
                            }
                            record.locked.store(false);
                        }
                    }

                    if (!running) {
                        break;
                    }

                    if (!processed) {
                        std::unique_lock<std::mutex> lock(mutex);
                        wakeCondition.wait(lock);
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
                    if((record.status & LOADED) || (record.status & FAILED_TO_LOAD)) {
                        if(!(record.status & SHOULD_NOT_LOAD) && !(record.status & FILE_NOT_FOUND)) {
                            if (record.timeStamp != 0) {
                                if (record.path != "") {
                                    uint64_t currentTimeStamp = getTimeStamp(record.path);
                                    if (currentTimeStamp != 0) {
                                        if (currentTimeStamp != record.timeStamp) {
                                            if(record.options & NO_RELOAD){
                                                continue;
                                            }
                                            if(record.options & NO_RELOAD_ONCE){
                                                record.options = (Options)((int)record.options & ~(int)NO_RELOAD_ONCE);
                                                record.timeStamp = currentTimeStamp;
                                                continue;
                                            }
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
                        if (record.preLoad == nullptr || record.preLoad(record.asset)) {
                            record.status = (Status) (record.status | STATE_PRE_LOADED);
                        } else {
                            record.status = (Status) (record.status | FAILED_TO_LOAD);
                            env->console->warning("failed to load asset: ", record.file);
                        }
                        processed = true;
                    }

                    if ((record.status & STATE_PRE_LOADED) && !(record.status & STATE_LOADED)) {
                        record.path = searchFile(record.file);
                        if (record.path == "") {
                            record.status = (Status) (record.status | FAILED_TO_LOAD);
                            record.status = (Status) (record.status | FILE_NOT_FOUND);
                            env->console->warning("asset file not found: ", record.file);
                        } else {
                            record.previousTimeStamp = record.timeStamp;
                            record.timeStamp = getTimeStamp(record.path);
                            TRI_PROFILE_PHASE("assets");
                            TRI_PROFILE("load");
                            {
                                TRI_PROFILE(record.file);
                                if (record.asset->load(record.path)) {
                                    record.status = (Status) (record.status | STATE_LOADED);
                                } else {
                                    record.status = (Status) (record.status | FAILED_TO_LOAD);
                                    env->console->warning("failed to load asset: ", record.file);
                                }
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
                        TRI_PROFILE_PHASE("assets");
                        TRI_PROFILE("activate");
                        {
                            TRI_PROFILE(record.file);
                            if (record.asset->loadActivate()) {
                                record.status = (Status) (record.status | STATE_ACTIVATED);
                            } else {
                                record.status = (Status) (record.status | FAILED_TO_LOAD);
                                env->console->warning("failed to load asset: ", record.file);
                            }
                            processed = true;
                        }
                    }

                    if ((record.status & STATE_ACTIVATED) && !(record.status & STATE_POST_LOADED)) {
                        if (record.postLoad == nullptr || record.postLoad(record.asset)) {
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

    std::vector<std::string> AssetManager::getAssetList(int typeId) {
        std::vector<std::string> list;
        for(auto &iter : assets){
            auto &record = iter.second;
            if(typeId == -1 || record.typeId == typeId){
                list.push_back(record.file);
            }
        }
        return list;
    }

    bool AssetManager::isLoadingInProcess(int typeId) {
        for (auto& iter : assets) {
            auto& record = iter.second;
            if (typeId == -1 || record.typeId == typeId) {
                if (!((record.status & LOADED) || (record.status & FAILED_TO_LOAD))) {
                    return true;
                }
            }
        }
        return false;
    }

}
