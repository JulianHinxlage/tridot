//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "EntityOperations.h"
#include "core/core.h"

namespace tri {

    EntityId EntityOperations::addEntity(Scene *scene) {
        if(scene == nullptr){
            scene = env->scene;
        }
        return scene->addEntity();
    }

    void EntityOperations::removeEntity(EntityId id, Scene *scene) {
        if(scene == nullptr){
            scene = env->scene;
        }
        scene->removeEntity(id);
    }

    void *EntityOperations::addComponent(int typeId, EntityId id, Scene *scene) {
        if(scene == nullptr){
            scene = env->scene;
        }
        return scene->addComponent(typeId, id);
    }

    void EntityOperations::removeComponent(int typeId, EntityId id, Scene *scene) {
        if(scene == nullptr){
            scene = env->scene;
        }
        scene->removeComponent(typeId, id);
    }

    EntityId EntityOperations::duplicateEntity(EntityId id, Scene *scene) {
        if(scene == nullptr){
            scene = env->scene;
        }
        EntityId copy = scene->addEntity();
        for(auto &desc : env->reflection->getDescriptors()){
            if(scene->hasComponent(desc->typeId, id)){
                desc->copy(scene->getComponent(desc->typeId, id), scene->addComponent(desc->typeId, copy));
            }
        }
        return copy;
    }

    void EntityOperations::copyEntity(EntityId id, Scene *scene) {
        if(scene == nullptr){
            scene = env->scene;
        }
        entityBuffer.copyEntity(id, scene);
        isEntityBufferFilled = true;
    }

    EntityId EntityOperations::pastEntity(Scene *scene) {
        if(scene == nullptr){
            scene = env->scene;
        }
        return entityBuffer.createEntity(scene);
    }

    void EntityOperations::copyComponent(int typeId, EntityId id, Scene *scene) {
        if(scene == nullptr){
            scene = env->scene;
        }
        componentBuffer.clear();
        if(scene->hasComponent(typeId, id)){
            componentBuffer.set(typeId, scene->getComponent(typeId, id));
        }
    }

    void EntityOperations::pastComponent(EntityId id, Scene *scene) {
        if(scene == nullptr){
            scene = env->scene;
        }
        if(componentBuffer.isSet()){
            componentBuffer.get(scene->addComponent(componentBuffer.getTypeId(), id));
        }
    }

    bool EntityOperations::wasEntityCopied() {
        return isEntityBufferFilled;
    }

    bool EntityOperations::wasComponentCopied() {
        return componentBuffer.isSet();
    }

}
