//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "ComponentCache.h"
#include "tridot/engine/Serializer.h"
#include "tridot/engine/Engine.h"

namespace tridot {

    bool ComponentCache::isCached(int typeId) {
        auto *type = Reflection::get(typeId);
        auto comp = data[type->name()];
        return (bool)comp;
    }

    bool ComponentCache::load(int typeId, void *ptr) {
        auto *type = Reflection::get(typeId);
        auto comp = data[type->name()];
        if(comp){
            Serializer s;
            s.deserializeType(type, comp, ptr, engine.resources);
            return true;
        }else{
            return false;
        }
    }

    void ComponentCache::remove(int typeId) {
        auto *type = Reflection::get(typeId);
        data.remove(type->name());
    }

    void ComponentCache::update(EntityId id) {
        for(auto &type : Reflection::getTypes()){
            if(type){
                if(isCached(type->id())){
                    if(!engine.has(id, type->id())){
                        void *ptr = engine.addByTypeId(id, type->id());
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
        engine.onUnregister().add("ComponentCache", [](int typeId){
            engine.view<>().each([&](EntityId id){
                if(typeId != Reflection::id<ComponentCache>()) {
                    if (engine.has(id, typeId)) {
                        if (!engine.has<ComponentCache>(id)) {
                            engine.add<ComponentCache>(id);
                        }
                        auto &cache = engine.get<ComponentCache>(id);
                        void *ptr = engine.get(id, typeId);
                        if (ptr) {
                            Serializer s;
                            YAML::Emitter out;
                            out << YAML::BeginMap;
                            auto *type = Reflection::get(typeId);
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
        engine.onUnregister().order({"ComponentCache", "Registry"});

        engine.onRegister().add("ComponentCache", [](int typeId){
            engine.view<ComponentCache>().each([](EntityId id, ComponentCache &cache){
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