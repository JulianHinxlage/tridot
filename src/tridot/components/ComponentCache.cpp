//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "ComponentCache.h"
#include "tridot/engine/Serializer.h"
#include "tridot/engine/Engine.h"

namespace tridot {

    bool ComponentCache::isCached(int typeId) {
        auto *type = env->reflection->getDescriptor(typeId);
        auto comp = data[type->name()];
        return (bool)comp;
    }

    bool ComponentCache::load(int typeId, void *ptr) {
        auto *type = env->reflection->getDescriptor(typeId);
        auto comp = data[type->name()];
        if(comp){
            Serializer s;
            s.deserializeType(type, comp, ptr, *env->resources);
            return true;
        }else{
            return false;
        }
    }

    void ComponentCache::remove(int typeId) {
        auto *type = env->reflection->getDescriptor(typeId);
        data.remove(type->name());
    }

    void ComponentCache::update(EntityId id) {
        for(auto &type : env->reflection->getDescriptors()){
            if(type){
                if(isCached(type->id())){
                    if(!env->scene->has(id, type->id())){
                        void *ptr = env->scene->addByTypeId(id, type->id());
                        if(ptr) {
                            load(type->id(), ptr);
                            remove(type->id());
                            if (data.size() == 0) {
                                env->scene->remove<ComponentCache>(id);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    TRI_INIT_CALLBACK("ComponentCache"){
        engine.onUnregister().add("ComponentCache", [](int typeId){
            env->scene->view<>().each([&](EntityId id){
                if(typeId != env->reflection->getTypeId<ComponentCache>()) {
                    if (env->scene->has(id, typeId)) {
                        if (!env->scene->has<ComponentCache>(id)) {
                            env->scene->add<ComponentCache>(id);
                        }
                        auto &cache = env->scene->get<ComponentCache>(id);
                        void *ptr = env->scene->get(id, typeId);
                        if (ptr) {
                            Serializer s;
                            YAML::Emitter out;
                            out << YAML::BeginMap;
                            auto *type = env->reflection->getDescriptor(typeId);
                            s.serializeType(type, type->name(), out, ptr, *env->resources);
                            out << YAML::EndMap;
                            YAML::Node node = YAML::Load(out.c_str());
                            for (auto comp : node) {
                                if (comp.first) {
                                    cache.data[comp.first] = comp.second;
                                }
                            }
                        }
                    }
                }
            });
        });
        engine.onUnregister().order({"ComponentCache", "Registry"});

        engine.onRegister().add("ComponentCache", [](int typeId){
            env->scene->view<ComponentCache>().each([](EntityId id, ComponentCache &cache){
                cache.update(id);
            });
        });
        engine.onRegister().order({"Registry", "ComponentCache"});

        engine.onShutdown().add("ComponentCache", [](){
            engine.onUnregister().remove("ComponentCache");
            engine.onRegister().remove("ComponentCache");
        });
    }

}