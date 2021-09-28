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
        if(scene->hasEntity(id)) {
            env->editor->undo.beginAction();

            //remove children
            for (auto &child : env->systems->getSystem<TransformSystem>()->getChildren(id)) {
                removeEntity(child, scene);
            }

            env->editor->undo.entityRemoved(id);
            scene->removeEntity(id);
            env->editor->undo.endAction();
        }
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
        env->editor->undo.beginAction();
        EntityId copy = scene->addEntity();
        for(auto &desc : env->reflection->getDescriptors()){
            if(scene->hasComponent(desc->typeId, id)){
                desc->copy(scene->getComponent(desc->typeId, id), scene->addComponent(desc->typeId, copy));
            }
        }
        env->editor->undo.entityAdded(copy);

        //duplicate children
        for(auto &child : env->systems->getSystem<TransformSystem>()->getChildren(id)){
            EntityId childCopy = duplicateEntity(child, scene);
            scene->update();
            if(scene->hasComponent<Transform>(childCopy)){
                scene->getComponent<Transform>(childCopy).parent = copy;
            }
        }

        env->editor->undo.endAction();
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

    void EntityOperations::duplicateSelection(Scene *scene) {
        if(scene == nullptr){
            scene = env->scene;
        }
        std::vector<EntityId> ids;
        env->editor->undo.beginAction();
        auto *transformSystem = env->systems->getSystem<TransformSystem>();
        for(auto &id : env->editor->selectionContext.getSelected()){

            bool isChildOfOtherSelected = false;
            for(auto &id2 : env->editor->selectionContext.getSelected()) {
                if(id != id2){
                    if(transformSystem->isParentOf(id, id2)){
                        isChildOfOtherSelected = true;
                        break;
                    }
                }
            }
            if(!isChildOfOtherSelected){
                ids.push_back(duplicateEntity(id, scene));
            }
        }
        env->editor->undo.endAction();
        env->editor->selectionContext.unselectAll();
        for(auto &id : ids){
            env->editor->selectionContext.select(id);
        }
    }

    void EntityOperations::removeSelection(Scene *scene) {
        if(scene == nullptr){
            scene = env->scene;
        }
        env->editor->undo.beginAction();
        for(auto &id : env->editor->selectionContext.getSelected()){
            removeEntity(id, scene);
        }
        env->editor->undo.endAction();
        env->editor->selectionContext.unselectAll();
    }

}
