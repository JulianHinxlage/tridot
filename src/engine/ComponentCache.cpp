//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "ComponentCache.h"
#include "core/core.h"
#include "Serializer.h"

namespace tri {

    TRI_REGISTER_TYPE(ComponentCache);

    TRI_STARTUP_CALLBACK("ComponentCache") {
        env->signals->typeRegister.addCallback("ComponentCache", [](int typeId) {
            if (env->editor) {
                env->scene->view<ComponentCache>().each([&](EntityId id, ComponentCache& cache) {
                    if (cache.isCached(typeId)) {
                        void* ptr = env->scene->addComponent(typeId, id);
                        cache.uncache(typeId, ptr, true);
                        if (cache.data.size() == 0) {
                            env->scene->removeComponent<ComponentCache>(id);
                        }
                    }
                });
            }
        });
        env->signals->typeUnregister.addCallback("ComponentCache", [](int typeId) {
            if (env->editor) {
                auto* pool = env->scene->getComponentPool(typeId);
                if (pool) {
                    for (int i = 0; i < pool->size(); i++) {
                        void* ptr = pool->getElementByIndex(i);
                        EntityId id = pool->getIdByIndex(i);
                        if (!env->scene->hasComponent<ComponentCache>(id)) {
                            ComponentCache& cache = env->scene->addComponent<ComponentCache>(id);
                            cache.cache(typeId, ptr);
                        }
                        else {
                            ComponentCache& cache = env->scene->getComponent<ComponentCache>(id);
                            cache.cache(typeId, ptr);
                        }
                    }
                }
            }
        });
    }

    void ComponentCache::cache(int typeId, void* ptr){
        YAML::Emitter out;
        env->serializer->serializeType(out, typeId, ptr);
        cache(typeId, out.c_str());
    }

    void ComponentCache::cache(int typeId, const std::string &str){
        auto& name = env->reflection->getType(typeId)->name;
        data[name] = str;
    }

    void ComponentCache::cache(const std::string& name, const std::string& str) {
        data[name] = str;
    }

    bool ComponentCache::uncache(int typeId, void* ptr, bool clear){
        if (isCached(typeId)) {
            auto& name = env->reflection->getType(typeId)->name;
            auto& str = data[name];
            YAML::Node node = YAML::Load(str);
            env->serializer->deserializeType(node, typeId, ptr);
            if (clear) {
                data.erase(name);
            }
            return true;
        }
        else {
            return false;
        }
    }

    bool ComponentCache::uncache(const std::string& name){
        if (isCached(name)) {
            data.erase(name);
            return true;
        }
        else {
            return false;
        }
    }

    bool ComponentCache::isCached(int typeId){
        auto &name = env->reflection->getType(typeId)->name;
        return data.find(name) != data.end();
    }

    bool ComponentCache::isCached(const std::string& name){
        return data.find(name) != data.end();
    }

}
