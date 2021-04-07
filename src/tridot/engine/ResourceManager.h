//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_RESOURCEMANAGER_H
#define TRIDOT_RESOURCEMANAGER_H

#include "tridot/core/Ref.h"
#include "tridot/core/Log.h"
#include <unordered_map>
#include <functional>
#include <thread>
#include <condition_variable>

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

        ResourceManager();
        ~ResourceManager();
        void addSearchDirectory(const std::string &directory);
        void update();

        template<typename T>
        Ref<T> get(const std::string &name, bool synchronous,
                   std::function<bool(T &t, const std::string &file)> preLoad,
                   std::function<bool(T &t)> postLoad){
            auto *res = ((ResourceT<T>*)getResource(name, typeid(T).hash_code(), synchronous, [preLoad, postLoad](){
                Ref<ResourceT<T>> resource = Ref<ResourceT<T>>::make();
                resource->ref = Ref<T>::make();
                ResourceT<T> *ptr = resource.get();
                if(preLoad != nullptr){
                    resource->preLoad = [ptr, preLoad](){return preLoad(*ptr->ref, ptr->file);};
                }
                if(postLoad != nullptr){
                    resource->postLoad = [ptr, postLoad](){return postLoad(*ptr->ref);};
                }
                return (Ref<Resource>)resource;
            }));
            if(res == nullptr){
                return nullptr;
            }else{
                return res->ref;
            }
        }

        template<typename T>
        Ref<T> get(const std::string &name, bool synchronous = false){
            if constexpr(has_member(T, postLoad) && has_member(T, preLoad)){
                return get<T>(name, synchronous,
                              [](T &t, const std::string &file){return t.preLoad(file);},
                              [](T &t){return t.postLoad();});
            }else{
                return get<T>(name, synchronous, nullptr, nullptr);
            }
        }

        template<typename T>
        Ref<T> &set(const std::string &name, bool synchronous = false){
            auto *res = ((ResourceT<T>*)getResource(name, typeid(T).hash_code(), synchronous, [](){
                Ref<ResourceT<T>> resource = Ref<ResourceT<T>>::make();
                resource->ref = Ref<T>::make();
                return (Ref<Resource>)resource;
            }));
            res->preLoaded = true;
            res->postLoaded = true;
            return res->ref;
        }

        template<typename T>
        std::vector<std::string> getList(){
            std::vector<std::string> list;
            uint32_t typeId = typeid(T).hash_code();
            for(auto &res : resources){
                if(res.second){
                    if(res.second->typeId == typeId){
                        list.push_back(res.second->name);
                    }
                }
            }
            return list;
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
            bool fileSearched;
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

#endif //TRIDOT_RESOURCEMANAGER_H
