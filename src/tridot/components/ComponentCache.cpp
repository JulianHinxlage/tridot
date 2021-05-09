//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "ComponentCache.h"
#include "tridot/engine/Serializer.h"
#include "tridot/engine/Engine.h"

namespace tridot {

    bool ComponentCache::isCached(int reflectId) {
        auto *type = Reflection::get(reflectId);
        auto comp = data[type->name()];
        return (bool)comp;
    }

    bool ComponentCache::load(int reflectId, void *ptr) {
        auto *type = Reflection::get(reflectId);
        auto comp = data[type->name()];
        if(comp){
            Serializer s;
            s.deserializeType(type, comp, ptr, engine.resources);
            return true;
        }else{
            return false;
        }
    }

    void ComponentCache::remove(int reflectId) {
        auto *type = Reflection::get(reflectId);
        data.remove(type->name());
    }

    void ComponentCache::update(EntityId id) {
        for(auto &type : Reflection::getTypes()){
            if(type){
                if(isCached(type->id())){
                    if(!engine.has(id, type->id())){
                        void *ptr = engine.addReflect(id, type->id());
                        if(ptr) {
                            load(type->id(), ptr);
                            remove(type->id());
                            if (data.size() == 0) {
                                engine.remove<ComponentCache>(id);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    TRI_INIT("ComponentCache"){
        engine.onUnregister().add("ComponentCache", [](int reflectId){
            engine.view<>().each([&](EntityId id){
                if(reflectId != Reflection::id<ComponentCache>()) {
                    if (engine.has(id, reflectId)) {
                        if (!engine.has<ComponentCache>(id)) {
                            engine.add<ComponentCache>(id);
                        }
                        auto &cache = engine.get<ComponentCache>(id);
                        void *ptr = engine.get(id, reflectId);
                        if (ptr) {
                            Serializer s;
                            YAML::Emitter out;
                            out << YAML::BeginMap;
                            auto *type = Reflection::get(reflectId);
                            s.serializeType(type, type->name(), out, ptr, engine.resources);
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

        engine.onRegister().add("ComponentCache", [](int reflectId){
            engine.view<ComponentCache>().each([](EntityId id, ComponentCache &cache){
                cache.update(id);
            });
        });

        engine.onShutdown().add("ComponentCache", [](){
            engine.onUnregister().remove("ComponentCache");
            engine.onRegister().remove("ComponentCache");
        });
    }

}