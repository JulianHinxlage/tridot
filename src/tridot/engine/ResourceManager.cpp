//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "ResourceManager.h"
#include <experimental/filesystem>

namespace tridot {

    ResourceManager::ResourceManager() {
        synchronousMode = false;
        autoReload = false;
        terminated = false;
    }

    ResourceManager::~ResourceManager() {
        terminated = true;
        if(thread){
            con.notify_all();
            thread->join();
        }
    }

    void ResourceManager::addSearchDirectory(const std::string &directory) {
        if(directory.back() == '/'){
            searchDirectories.push_back(directory);
        }else{
            searchDirectories.push_back(directory + "/");
        }
    }

    void ResourceManager::update() {
        if(!synchronousMode){
            if(!thread){
                thread = Ref<std::thread>::make([this](){
                    while(!terminated) {
                        for (auto &res : resources) {
                            preUpdate(res.second.get());
                        }
                        std::unique_lock lock(mutex);
                        con.wait_for(lock, std::chrono::milliseconds(500));
                    }
                });
            }
        }
        for(auto &res : resources){
            postUpdate(res.second.get());
        }
    }

    ResourceManager::Resource *ResourceManager::getResource(const std::string &name, uint32_t typeId, bool synchronous, std::function<Ref<Resource>()> create) {
        auto entry = resources.find(name);
        if(entry == resources.end()){
            Ref<Resource> res = create();
            res->typeId = typeId;
            res->name = name;
            res->timestamp = 0;
            res->file = "";
            res->preLoaded = false;
            res->postLoaded = false;
            res->fileSearched = false;
            resources[name] = res;

            if(synchronous || synchronousMode){
                preUpdate(res.get());
                postUpdate(res.get());
            }else{
                con.notify_one();
            }

            return res.get();
        }else{
            if(typeId != entry->second->typeId){
                Log::warning("requested different type for resource ", entry->second->name);
                return nullptr;
            }else{
                return entry->second.get();
            }
        }
    }

    uint64_t getTimestamp(const std::string &file){
        if(!std::experimental::filesystem::exists(file)){
            return 0;
        }else{
            return std::experimental::filesystem::last_write_time(file).time_since_epoch().count();
        }
    }

    void ResourceManager::preUpdate(Resource *res) {
        if (!res->preLoaded) {
            bool found = false;
            for(auto &dir : searchDirectories){
                std::string file = dir + res->name;
                if(std::experimental::filesystem::exists(file)){
                    found = true;
                    res->file = file;
                    if(!res->preLoad){
                        Log::warning("no preLoad callback provided for resource \"", res->name, "\"");
                        res->timestamp = getTimestamp(file);
                        res->preLoaded = true;
                        res->postLoaded = false;
                        break;
                    }else if(res->preLoad()){
                        res->timestamp = getTimestamp(file);
                        res->preLoaded = true;
                        res->postLoaded = false;
                        break;
                    }else{
                        res->file = "";
                    }
                }
            }
            if(!found && !res->fileSearched){
                Log::warning("resource \"", res->name, "\" not found");
            }
            res->fileSearched = true;
        }else{
            if(res->postLoaded && autoReload) {
                uint64_t timestamp = getTimestamp(res->file);
                if (timestamp != res->timestamp) {
                    if (timestamp == 0) {
                        res->preLoaded = false;
                        res->postLoaded = false;
                    } else {
                        if(!res->preLoad){
                            Log::warning("no preLoad callback provided for resource \"", res->name, "\"");
                            res->timestamp = timestamp;
                            res->preLoaded = true;
                            res->postLoaded = false;
                        }else if (res->preLoad()) {
                            res->timestamp = timestamp;
                            res->preLoaded = true;
                            res->postLoaded = false;
                        }
                    }
                }
            }
        }
    }

    void ResourceManager::postUpdate(Resource *res) {
        if(res->preLoaded && !res->postLoaded){
            if(!res->postLoad){
                Log::warning("no postLoad callback provided for resource \"", res->name, "\"");
                res->postLoaded = true;
            }else if(res->postLoad()){
                res->postLoaded = true;
            }
        }
    }

}