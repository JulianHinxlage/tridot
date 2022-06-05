//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "UndoSystem.h"
#include "Editor.h"

namespace tri {

    TRI_SYSTEM(UndoSystem);

    void UndoSystem::init() {
        env->editor->undo = this;
        env->jobManager->addJob("Render")->addSystem<UndoSystem>();
    }

    void UndoSystem::shutdown() {
        clear();
    }

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
                env->world->removeEntity(action.id);
                break;
            }
            case Action::ENTITY_REMOVED: {
                EntityId id = action.entity.createEntity(env->world, action.id);
                entityAdded(id);
                break;
            }
            case Action::COMP_ADDED: {
                componentRemoved(action.classId, action.id);
                env->world->removeComponent(action.id, action.classId);
                break;
            }
            case Action::COMP_REMOVED: {
                action.comp.get(env->world->addComponent(action.id, action.classId));
                componentAdded(action.classId, action.id);
                break;
            }
            case Action::COMP_CHANGED: {
                if(env->world->hasComponent(action.id, action.classId)){
                    void *comp = env->world->getComponent(action.id, action.classId);
                    componentChanged(action.classId, action.id, comp);
                    action.comp.get(comp);
                }
                break;
            }
            case Action::VALUE_CHANGED: {
                valueChanged(action.classId, action.instance, action.instance);
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
        action->entity.copyEntity(id, env->world);

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

    void UndoSystem::componentChanged(int classId, EntityId id, void *preEditValue) {
        Ref<Action> action = Ref<Action>::make();
        action->id = id;
        action->classId = classId;
        action->op = Action::COMP_CHANGED;
        if(inActionCounter > 0){
            action->isActionSequence = inActionWasAdded;
            inActionWasAdded = true;
        }
        action->comp.set(classId, preEditValue);

        if(addToRedo){
            redoActions.push_back(action);
        }else{
            undoActions.push_back(action);
            if(clearRedoOnAction){
                redoActions.clear();
            }
        }
    }

    void UndoSystem::componentRemoved(int classId, EntityId id) {
        Ref<Action> action = Ref<Action>::make();
        action->id = id;
        action->classId = classId;
        action->op = Action::COMP_REMOVED;
        if(inActionCounter > 0){
            action->isActionSequence = inActionWasAdded;
            inActionWasAdded = true;
        }
        action->comp.set(classId, env->world->getComponent(id, classId));

        if(addToRedo){
            redoActions.push_back(action);
        }else{
            undoActions.push_back(action);
            if(clearRedoOnAction){
                redoActions.clear();
            }
        }
    }

    void UndoSystem::componentAdded(int classId, EntityId id) {
        Ref<Action> action = Ref<Action>::make();
        action->id = id;
        action->classId = classId;
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

    void UndoSystem::valueChanged(int classId, void *instance, void *preEditValue) {
        Ref<Action> action = Ref<Action>::make();
        action->classId = classId;
        action->op = Action::VALUE_CHANGED;
        if(inActionCounter > 0){
            action->isActionSequence = inActionWasAdded;
            inActionWasAdded = true;
        }
        action->comp.set(classId, preEditValue);
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
