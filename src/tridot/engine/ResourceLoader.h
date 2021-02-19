//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_RESOURCELOADER_H
#define TRIDOT_RESOURCELOADER_H

#include "tridot/core/Ref.h"
#include "tridot/core/Log.h"
#include <unordered_map>
#include <functional>
#include <thread>
#include <condition_variable>

namespace tridot {

    class ResourceLoader {
    public:
        bool synchronousMode;
        bool autoReload;

        ResourceLoader();
        ~ResourceLoader();
        void addSearchDirectory(const std::string &directory);
        void update();

        //just don't try to read this function
        template<typename T>
        Ref<T> get(const std::string &name, bool synchronous = false,
                   std::function<bool(T &t, const std::string &file)> preLoad = [](T &t, const std::string &file){return t.preLoad(file);},
                   std::function<bool(T &t)> postLoad = [](T &t){return t.postLoad();}){
            auto *res = ((ResourceT<T>*)getResource(name, typeid(T).hash_code(), synchronous, [preLoad, postLoad](){
                Ref<ResourceT<T>> resource = Ref<ResourceT<T>>::make();
                resource->ref = Ref<T>::make();
                ResourceT<T> *ptr = resource.get();
                resource->preLoad = [ptr, preLoad](){return preLoad(*ptr->ref, ptr->file);};
                resource->postLoad = [ptr, postLoad](){return postLoad(*ptr->ref);};
                return (Ref<Resource>)resource;
            }));
            if(res == nullptr){
                return nullptr;
            }else{
                return res->ref;
            }
        }

    private:
        class Resource{
        public:
            std::string name;
            std::string file;
            uint32_t typeId;
            uint64_t timestamp;
            bool preLoaded;
            bool postLoaded;
            std::function<bool()> preLoad;
            std::function<bool()> postLoad;
        };

        template<typename T>
        class ResourceT : public Resource{
        public:
            Ref<T> ref;
        };

        Resource *getResource(const std::string &name, uint32_t typeId, bool synchronous, std::function<Ref<Resource>()> create);
        void preUpdate(Resource *res);
        void postUpdate(Resource *res);

        std::vector<std::string> searchDirectories;
        std::unordered_map<std::string, Ref<Resource>> resources;
        Ref<std::thread> thread;
        std::condition_variable con;
        std::mutex mutex;
        bool terminated;
    };

}

#endif //TRIDOT_RESOURCELOADER_H
