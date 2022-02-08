//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Prefab.h"
#include "engine/Serializer.h"
#include "engine/Transform.h"

namespace tri {
    
    TRI_REGISTER_ASSET(Prefab);

    Prefab::Prefab(){

    }

    Prefab::Prefab(const Prefab& prefab) {
        operator=(prefab);
    }

    void *Prefab::addComponent(int typeId) {
        if (void* data = getComponent(typeId)) {
            return data;
        }
        else {
            ComponentBuffer& comp = comps.emplace_back(typeId);
            return comp.get();
        }
    }

    void *Prefab::getComponent(int typeId) {
        for (int i = 0; i < comps.size(); i++) {
            if (comps[i].getTypeId() == typeId) {
                return comps[i].get();
            }
        }
        return nullptr;
    }

    bool Prefab::hasComponent(int typeId) {
        return getComponent(typeId) != nullptr;
    }

    bool Prefab::removeComponent(int typeId) {
        for (int i = 0; i < comps.size(); i++) {
            if (comps[i].getTypeId() == typeId) {
                comps.erase(comps.begin() + i);
                return true;
            }
        }
        return false;
    }

    EntityId Prefab::createEntity(Scene* scene, EntityId hint) {
        EntityId id = scene->addEntityHinted(hint);
        for (int i = 0; i < comps.size(); i++) {
            ComponentBuffer& comp = comps[i];
            void *ptr = scene->addComponent(comp.getTypeId(), id);
            comp.get(ptr);
        }
        for(auto &child : getChildren()){
            EntityId childId = child->createEntity(scene);
            if (Transform* t = scene->getPendingComponent<Transform>(childId)) {
                t->parent = id;
            }
        }
        return id;
    }

    void Prefab::copyEntity(EntityId id, Scene *scene, bool copyChildren){
        clear();
        for(auto &desc : env->reflection->getDescriptors()){
            if(desc && scene->hasComponent(desc->typeId, id)){
                desc->copy(scene->getComponent(desc->typeId, id), addComponent(desc->typeId));
            }
        }
        if(copyChildren){
            for(auto child : env->hierarchies->getChildren(id)){
                addChild().copyEntity(child, scene);
            }
        }
    }

    void Prefab::operator=(const Prefab& prefab) {
        comps = prefab.comps;
        children.resize(prefab.children.size());
        for (int i = 0; i < children.size(); i++) {
            children[i] = Ref<Prefab>::make(*prefab.children[i]);
        }
    }

    void Prefab::clear(){
        comps.clear();
        children.clear();
    }

    bool Prefab::load(const std::string &file) {
        std::ifstream stream(file);
        if(stream.is_open()){
            YAML::Node in = YAML::Load(stream);
            env->serializer->deserializePrefab(in, *this);
            return true;
        }
        return false;
    }

    bool Prefab::save(const std::string &file) {
        std::ofstream stream(file);
        if(stream.is_open()){
            YAML::Emitter out(stream);
            env->serializer->serializePrefab(out, *this);
            return true;
        }
        return false;
    }

}
