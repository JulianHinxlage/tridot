//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "EntityOperations.h"
#include "core/core.h"
#include "Editor.h"

namespace tri {

    EntityId EntityOperations::addEntity(Scene *scene) {
        if(scene == nullptr){
            scene = env->scene;
        }
        EntityId id = scene->addEntity();
        env->editor->undo.entityAdded(id);
        return id;
    }

    void EntityOperations::removeEntity(EntityId id, Scene *scene) {
        if(scene == nullptr){
            scene = env->scene;
        }
        env->editor->undo.entityRemoved(id);
        scene->removeEntity(id);
    }

    void *EntityOperations::addComponent(int typeId, EntityId id, Scene *scene) {
        if(scene == nullptr){
            scene = env->scene;
        }
        void *comp = scene->addComponent(typeId, id);
        env->editor->undo.componentAdded(typeId, id);
        return comp;
    }

    void EntityOperations::removeComponent(int typeId, EntityId id, Scene *scene) {
        if(scene == nullptr){
            scene = env->scene;
        }
        env->editor->undo.componentRemoved(typeId, id);
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
        env->editor->undo.entityAdded(copy);
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
        EntityId id = entityBuffer.createEntity(scene);
        env->editor->undo.entityAdded(id);
        return id;
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
            env->editor->undo.componentAdded(componentBuffer.getTypeId(), id);
        }
    }

    bool EntityOperations::wasEntityCopied() {
        return isEntityBufferFilled;
    }

    bool EntityOperations::wasComponentCopied() {
        return componentBuffer.isSet();
    }

}
