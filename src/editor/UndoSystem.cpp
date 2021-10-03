//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "UndoSystem.h"

namespace tri {

    void UndoSystem::undo() {
        clearRedoOnAction = false;
        addToRedo = true;
        beginAction();
        while(!undoActions.empty()){
            Action &action = *undoActions.back().get();
            undoAction(action);
            if(!action.isActionSequence){
                undoActions.pop_back();
                break;
            }
            undoActions.pop_back();
        }
        endAction();
        addToRedo = false;
        clearRedoOnAction = true;
    }

    void UndoSystem::redo() {
        clearRedoOnAction = false;
        beginAction();
        while(!redoActions.empty()){
            Action &action = *redoActions.back().get();
            undoAction(action);
            if(!action.isActionSequence){
                redoActions.pop_back();
                break;
            }
            redoActions.pop_back();
        }
        endAction();
        clearRedoOnAction = true;
    }

    void UndoSystem::undoAction(Action &action) {
        switch(action.op){
            case Action::ENTITY_ADDED: {
                entityRemoved(action.id);
                env->scene->removeEntity(action.id);
                break;
            }
            case Action::ENTITY_REMOVED: {
                EntityId id = action.entity.createEntity(env->scene, action.id);
                entityAdded(id);
                break;
            }
            case Action::COMP_ADDED: {
                componentRemoved(action.typeId, action.id);
                env->scene->removeComponent(action.typeId, action.id);
                break;
            }
            case Action::COMP_REMOVED: {
                action.comp.get(env->scene->addComponent(action.typeId, action.id));
                componentAdded(action.typeId, action.id);
                break;
            }
            case Action::COMP_CHANGED: {
                if(env->scene->hasComponent(action.typeId, action.id)){
                    void *comp = env->scene->getComponent(action.typeId, action.id);
                    componentChanged(action.typeId, action.id, comp);
                    action.comp.get(comp);
                }
                break;
            }
            case Action::VALUE_CHANGED: {
                valueChanged(action.typeId, action.instance, action.instance);
                action.comp.get(action.instance);
                break;
            }
        }
    }

    void UndoSystem::clear() {
        undoActions.clear();
        redoActions.clear();
    }

    void UndoSystem::beginAction() {
        if(inActionCounter == 0){
            inActionWasAdded = false;
        }
        inActionCounter++;
    }

    void UndoSystem::endAction() {
        std::max(0, inActionCounter--);
        if(inActionCounter == 0){
            inActionWasAdded = false;
        }
    }

    void UndoSystem::entityRemoved(EntityId id) {
        Ref<Action> action = Ref<Action>::make();
        action->id = id;
        action->op = Action::ENTITY_REMOVED;
        if(inActionCounter > 0){
            action->isActionSequence = inActionWasAdded;
            inActionWasAdded = true;
        }
        action->entity.copyEntity(id, env->scene, false);

        if(addToRedo){
            redoActions.push_back(action);
        }else{
            undoActions.push_back(action);
            if(clearRedoOnAction){
                redoActions.clear();
            }
        }
    }

    void UndoSystem::entityAdded(EntityId id) {
        Ref<Action> action = Ref<Action>::make();
        action->id = id;
        action->op = Action::ENTITY_ADDED;
        if(inActionCounter > 0){
            action->isActionSequence = inActionWasAdded;
            inActionWasAdded = true;
        }

        if(addToRedo){
            redoActions.push_back(action);
        }else{
            undoActions.push_back(action);
            if(clearRedoOnAction){
                redoActions.clear();
            }
        }
    }

    void UndoSystem::componentChanged(int typeId, EntityId id, void *preEditValue) {
        Ref<Action> action = Ref<Action>::make();
        action->id = id;
        action->typeId = typeId;
        action->op = Action::COMP_CHANGED;
        if(inActionCounter > 0){
            action->isActionSequence = inActionWasAdded;
            inActionWasAdded = true;
        }
        action->comp.set(typeId, preEditValue);

        if(addToRedo){
            redoActions.push_back(action);
        }else{
            undoActions.push_back(action);
            if(clearRedoOnAction){
                redoActions.clear();
            }
        }
    }

    void UndoSystem::componentRemoved(int typeId, EntityId id) {
        Ref<Action> action = Ref<Action>::make();
        action->id = id;
        action->typeId = typeId;
        action->op = Action::COMP_REMOVED;
        if(inActionCounter > 0){
            action->isActionSequence = inActionWasAdded;
            inActionWasAdded = true;
        }
        action->comp.set(typeId, env->scene->getComponent(typeId, id));

        if(addToRedo){
            redoActions.push_back(action);
        }else{
            undoActions.push_back(action);
            if(clearRedoOnAction){
                redoActions.clear();
            }
        }
    }

    void UndoSystem::componentAdded(int typeId, EntityId id) {
        Ref<Action> action = Ref<Action>::make();
        action->id = id;
        action->typeId = typeId;
        action->op = Action::COMP_ADDED;
        if(inActionCounter > 0){
            action->isActionSequence = inActionWasAdded;
            inActionWasAdded = true;
        }

        if(addToRedo){
            redoActions.push_back(action);
        }else{
            undoActions.push_back(action);
            if(clearRedoOnAction){
                redoActions.clear();
            }
        }
    }

    void UndoSystem::valueChanged(int typeId, void *instance, void *preEditValue) {
        Ref<Action> action = Ref<Action>::make();
        action->typeId = typeId;
        action->op = Action::VALUE_CHANGED;
        if(inActionCounter > 0){
            action->isActionSequence = inActionWasAdded;
            inActionWasAdded = true;
        }
        action->comp.set(typeId, preEditValue);
        action->instance = instance;

        if(addToRedo){
            redoActions.push_back(action);
        }else{
            undoActions.push_back(action);
            if(clearRedoOnAction){
                redoActions.clear();
            }
        }
    }

}
