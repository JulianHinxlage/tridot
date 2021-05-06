//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_RESOURCEMANAGER_H
#define TRIDOT_RESOURCEMANAGER_H

#include "tridot/core/Ref.h"
#include "tridot/core/Log.h"
#include <map>
#include <functional>
#include <thread>
#include <condition_variable>
#include <filesystem>
#include <algorithm>

#define define_has_member(member_name)                                         \
    template <typename T>                                                      \
    class has_member_##member_name                                             \
    {                                                                          \
        typedef char yes_type;                                                 \
        typedef long no_type;                                                  \
        template <typename U> static yes_type test(decltype(&U::member_name)); \
        template <typename U> static no_type  test(...);                       \
    public:                                                                    \
        static constexpr bool value = sizeof(test<T>(0)) == sizeof(yes_type);  \
    };
#define has_member(type, member_name)  has_member_##member_name<type>::value

define_has_member(preLoad)
define_has_member(postLoad)

namespace tridot {

    class ResourceManager {
    public:
        bool synchronousMode;
        bool autoReload;
        int threadCount;

        enum Options {
            NONE = 0,
            SYNCHRONOUS = 1,
            JUST_CREATE = 2,
            LOAD_WITHOUT_FILE = 4,
        };

        enum Status {
            UNKNOWN = 0,
            UNCREATED,
            UNLOADED,
            PRE_LOADED,
            LOADED,
            FILE_NOT_FOUND,
            FAILED_TO_LOAD,
        };

        template<typename T>
        class ResourceOptions {
        public:
            std::string name;
            std::string file;
            int options;
            std::function<bool(Ref<T> &, const std::string &)> preLoad;
            std::function<bool(Ref<T> &)> postLoad;
            std::function<Ref<T>()> create;
            ResourceManager *manager;

            ResourceOptions(){
                manager = nullptr;
                name = "";
                file = "";
                options = NONE;
                if constexpr(has_member(T, preLoad)){
                    preLoad = [](Ref<T> &t, const std::string &file){return t->preLoad(file);};
                }else{
                    preLoad = nullptr;
                }
                if constexpr(has_member(T, postLoad)){
                    postLoad = [](Ref<T> &t){return t->postLoad();};
                }else{
                    postLoad = nullptr;
                }
                create = [](){return Ref<T>::make();};
            }

            ResourceOptions &setFile(const std::string &file) {
                this->file = file;
                return *this;
            }

            ResourceOptions &setOptions(int options) {
                this->options = options;
                return *this;
            }

            ResourceOptions &addOptions(int options) {
                this->options |= options;
                return *this;
            }

            ResourceOptions &setPreLoad(const std::function<bool(Ref<T> &, const std::string &)> &preLoad) {
                this->preLoad = preLoad;
                return *this;
            }

            ResourceOptions &setPostLoad(const std::function<bool(Ref<T> &)> &postLoad) {
                this->postLoad = postLoad;
                return *this;
            }

            ResourceOptions &setCreate(const std::function<Ref<T>()> &create) {
                this->create = create;
                return *this;
            }

            ResourceOptions &setInstance(T *instance) {
                this->create = [instance](){return std::shared_ptr<T>(instance, [](T*){});};
                return *this;
            }

            ResourceOptions &setInstance(Ref<T> &instance) {
                Ref<T> *ptr = &instance;
                this->create = [ptr](){return *ptr;};
                return *this;
            }

            Ref<T> get(){
                return manager->get<T>(name);
            }
        };

        ResourceManager() {
            synchronousMode = false;
            autoReload = true;
            threadCount = 1;
        }

        ~ResourceManager() {
            terminated = true;
            condition.notify_all();
            for(auto &thread : threads){
                if(thread){
                    thread->join();
                }
            }
        }

        void update(){
            if(synchronousMode){
                for(auto &entry : entries) {
                    entry.second->pre();
                }
            }else if(threads.size() != threadCount){
                threads.resize(threadCount);
                for(int i = 0; i < threads.size(); i++){
                    if(threads[i].get() == nullptr){
                        threads[i] = Ref<std::thread>::make([this](){
                            while(!terminated) {
                                for (auto &entry : entries) {
                                    if(entry.second->mutex.try_lock()){
                                        entry.second->pre();
                                        entry.second->mutex.unlock();
                                    }
                                }
                                std::unique_lock lock(mutex);
                                condition.wait_for(lock, std::chrono::milliseconds(1000));
                            }
                        });
                    }
                }
            }
            for(auto &entry : entries){
                entry.second->post();
            }
        }

        void addSearchDirectory(const std::string &directory){
            if(!directory.empty() && directory.back() != '/'){
                searchDirectories.push_back(directory + "/");
            }else{
                searchDirectories.push_back(directory);
            }
        }

        template<typename T>
        ResourceOptions<T> &setup(const std::string &name) {
            EntryBase *entry = getEntry(name);
            if(entry != nullptr){
                uint32_t type = typeid(T).hash_code();
                if(entry->type != type){
                    if(entry->status & UNCREATED){
                        remove(name);
                        return setup<T>(name);
                    }else{
                        Log::warning("resource ", name, " requested with different type");
                        static ResourceOptions<T> options;
                        return options;
                    }
                }
            }else{
                Ref<Entry<T>> res = Ref<Entry<T>>::make();
                res->options = defaultOptions<T>();
                res->options.manager = this;
                res->options.name = name;
                res->options.file = name;
                entries[name] = (Ref<EntryBase>)res;
                entry = res.get();
            }
            return ((Entry<T>*)entry)->options;
        }

        template<typename T>
        Ref<T> get(const std::string &name){
            EntryBase *entry = getEntry(name);
            if(entry == nullptr){
                setup<T>(name);
                entry = getEntry(name);
                if(entry == nullptr){
                    return nullptr;
                }
            }
            uint32_t type = typeid(T).hash_code();
            if(entry->type != type){
                if(entry->status & UNCREATED){
                    remove(name);
                    return get<T>(name);
                }else{
                    Log::warning("resource ", name, " requested with different type");
                    return nullptr;
                }
            }
            Entry<T> *res = ((Entry<T>*)entry);
            res->create();
            if(synchronousMode || res->options.options & SYNCHRONOUS){
                res->mutex.lock();
                res->pre();
                res->post();
                res->mutex.unlock();
            }else{
                condition.notify_one();
            }
            return res->resource;
        }

        template<typename T>
        Ref<T> get(const std::string &name, int options){
            return setup<T>(name).setOptions(options).get();
        }

        template<typename T>
        std::string getName(Ref<T> &resource){
            uint32_t type = typeid(T).hash_code();
            for(auto &entry : entries){
                if(entry.second->type == type){
                    Entry <T> *res = ((Entry <T> *) entry.second.get());
                    if(res->resource != nullptr){
                        if (res->resource == resource) {
                            return entry.first;
                        }
                    }
                }
            }
            return "";
        }

        Status getStatus(const std::string &name){
            EntryBase *entry = getEntry(name);
            if(entry == nullptr){
                return UNKNOWN;
            }else{
                return entry->status;
            }
        }

        bool isUsed(const std::string &name){
            if(EntryBase *entry = getEntry(name)){
                return entry->isUsed();
            }else{
                return false;
            }
        }

        void release(const std::string &name){
            if(EntryBase *entry = getEntry(name)){
                entry->release();
            }
        }

        void remove(const std::string &name){
            entries.erase(name);
        }

        template<typename T>
        std::vector<std::string> getNameList(bool alphabetical = false){
            std::vector<std::string> list;
            uint32_t type = typeid(T).hash_code();
            for(auto &entry : entries){
                if(entry.second->type == type){
                    list.push_back(entry.first);
                }
            }
            if(alphabetical){
                std::sort(list.begin(), list.end());
            }
            return list;
        }

        std::string searchFile(const std::string &file){
            for(auto &dir : searchDirectories) {
                if(std::filesystem::exists(dir + file)){
                    return dir + file;
                }
            }
            return "";
        }

        void releaseAllUnused(bool remove = false){
            for(auto &entry : entries){
                if(entry.second->status != UNCREATED){
                    if(!entry.second->isUsed()){
                        if(remove){
                            entries.erase(entry.first);
                        }else{
                            entry.second->release();
                        }
                    }
                }
            }
        }

        bool isLoading(){
            for(auto &entry : entries){
                if(entry.second->status == UNLOADED || entry.second->status == PRE_LOADED){
                    return true;
                }
            }
            return false;
        }

        template<typename T>
        ResourceOptions<T> &defaultOptions(){
            uint32_t type = typeid(T).hash_code();
            auto entry = defaultOptionEntries.find(type);
            if(entry == defaultOptionEntries.end()){
                Ref<Entry<T>> res = Ref<Entry<T>>::make();
                defaultOptionEntries[type] = (Ref<EntryBase>)res;
                return res->options;
            }else{
                Entry<T> *res = (Entry<T>*)entry->second.get();
                return res->options;

            }
        }

    private:
        class EntryBase{
        public:
            Status status;
            uint32_t type;
            std::string filePath;
            uint64_t timestamp;
            std::mutex mutex;

            virtual void create() = 0;
            virtual void pre() = 0;
            virtual void post() = 0;
            virtual bool isUsed() = 0;
            virtual void release() = 0;
        };

        template<typename T>
        class Entry : public EntryBase{
        public:
            ResourceOptions<T> options;
            Ref<T> resource;

            Entry(){
                type = typeid(T).hash_code();
                resource = nullptr;
                status = UNCREATED;
            }

            static uint64_t getTimestamp(const std::string &file){
                if(!std::filesystem::exists(file)){
                    return 0;
                }else{
                    return std::filesystem::last_write_time(file).time_since_epoch().count();
                }
            }

            virtual void create(){
                if(resource == nullptr){
                    if(options.create){
                        resource = options.create();
                        status = UNLOADED;
                        if(options.options & JUST_CREATE){
                            status = LOADED;
                        }
                    }else{
                        Log::warning("no create function for resource ", options.name);
                    }
                }
            }

            virtual void pre(){
                if(status == UNLOADED){
                    if(options.preLoad){
                        if(options.options & LOAD_WITHOUT_FILE){
                            if(options.preLoad(resource, "")){
                                status = PRE_LOADED;
                            }else{
                                status = FAILED_TO_LOAD;
                                Log::warning("failed to load resource ", options.name);
                            }
                        }else {
                            bool found = false;
                            for (auto &directory : options.manager->searchDirectories) {
                                filePath = directory + options.file;
                                timestamp = getTimestamp(filePath);
                                if (timestamp != 0) {
                                    found = true;
                                    if (options.preLoad(resource, filePath)) {
                                        status = PRE_LOADED;
                                        break;
                                    }
                                }
                            }
                            if(status != PRE_LOADED){
                                timestamp = 0;
                                filePath = "";
                                if(found){
                                    status = FAILED_TO_LOAD;
                                    Log::warning("failed to load resource ", options.name);
                                }else{
                                    status = FILE_NOT_FOUND;
                                    Log::warning("file not found for resource ", options.name);
                                }
                            }
                        }
                    }else{
                        status = PRE_LOADED;
                    }
                }else{
                    if(options.manager->autoReload && status == LOADED){
                        if(!(options.options & LOAD_WITHOUT_FILE) && !(options.options & JUST_CREATE)){
                            if(timestamp != 0){
                                uint64_t fileTimestamp = getTimestamp(filePath);
                                if(timestamp != fileTimestamp){
                                    status = UNLOADED;
                                    pre();
                                }
                            }
                        }
                    }
                }
            }

            virtual void post(){
                if(status == PRE_LOADED){
                    if(options.postLoad){
                        if(options.postLoad(resource)){
                            status = LOADED;
                        }else{
                            status = FAILED_TO_LOAD;
                            Log::warning("failed to load resource ", options.name);
                        }
                    }else{
                        status = LOADED;
                    }
                }
            }

            virtual bool isUsed(){
                return resource.use_count() > 1;
            }

            virtual void release(){
                if(resource != nullptr){
                    Log::debug("released resource ", options.name);
                }
                resource = nullptr;
                status = UNCREATED;
                filePath = "";
                timestamp = 0;
            }

        };

        EntryBase *getEntry(const std::string &name){
            auto entry = entries.find(name);
            if(entry == entries.end()){
                return nullptr;
            }else{
                return entry->second.get();
            }
        }

        std::unordered_map<std::string, Ref<EntryBase>> entries;
        std::unordered_map<uint32_t, Ref<EntryBase>> defaultOptionEntries;
        std::vector<Ref<std::thread>> threads;
        std::vector<std::string> searchDirectories;
        std::mutex mutex;
        std::condition_variable condition;
        bool terminated;
    };

}

#endif //TRIDOT_RESOURCEMANAGER_H
