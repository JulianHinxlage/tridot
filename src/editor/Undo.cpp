//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Undo.h"
#include "Editor.h"

namespace tridot {

    void Undo::addCustomAction(const std::function<void()>& undo, const std::function<void()>& redo){
        if(enabled){
            actionBuffer.undos.push_back(undo);
            actionBuffer.redos.push_back(redo);
        }
    }

    void Undo::undoAction(){
        if(enabled && nextAction - 1 < actions.size()){
            for(auto &undo : actions[nextAction-1].undos){
                if(undo){
                    undo();
                }
            }
            for(auto &comp : actions[nextAction-1].components){
                EntityId id = comp.id;
                if(!env->scene->exists(id)){
                    id = env->scene->createHinted(id);
                }
                if(!env->scene->has(id, comp.typeId)) {
                    env->scene->addByTypeId(id, comp.typeId);
                }
                void *data = env->scene->get(id, comp.typeId);
                env->reflection->getDescriptor(comp.typeId)->copy(comp.startData.get(), data);
            }
            nextAction--;
        }
    }

    void Undo::redoAction(){
        if(enabled && nextAction < actions.size()){
            for(auto &redo : actions[nextAction].redos){
                if(redo){
                    redo();
                }
            }
            for(auto &comp : actions[nextAction].components){
                if(env->scene->has(comp.id, comp.typeId)){
                    void *data = env->scene->get(comp.id, comp.typeId);
                    env->reflection->getDescriptor(comp.typeId)->copy(comp.endData.get(), data);
                }
            }
            nextAction++;
        }
    }

    void Undo::beginAction() {
        if(enabled){
            inAction = true;
        }
    }

    void Undo::endAction() {
        if(enabled) {
            if (actionBuffer.undos.size() > 0 || actionBuffer.redos.size() > 0 || actionBuffer.components.size() > 0) {
                while (nextAction < actions.size()) {
                    actions.pop_back();
                }
                actions.push_back(actionBuffer);
                nextAction++;
                actionBuffer.undos.clear();
                actionBuffer.redos.clear();
                actionBuffer.components.clear();
            }
            inAction = false;
        }
    }

    void Undo::destroyEntity(EntityId id) {
        if(enabled) {
            for (auto &type : env->reflection->getDescriptors()) {
                if (env->scene->has(id, type->id())) {
                    changeComponent(id, type, env->scene->get(id, type->id()));
                }
            }
            addCustomAction(nullptr, [id]() {
                env->scene->destroy(id);
            });
            if (!inAction) {
                endAction();
            }
        }
    }

    void Undo::duplicateEntity(EntityId id, EntityId newId) {
        if(enabled) {
            addCustomAction([newId]() {
                env->scene->destroy(newId);
            }, [id]() {
                env->editor->selection.duplicate(id, false);
            });
            if (!inAction) {
                endAction();
            }
        }
    }

    void Undo::changeComponent(EntityId id, Reflection::TypeDescriptor *type, void *value) {
        if(enabled) {
            for (auto &comp : actionBuffer.components) {
                if (comp.id == id && comp.typeId == type->id()) {
                    type->copy(value, comp.endData.get());
                    return;
                }
            }
            actionBuffer.components.push_back({});
            auto &comp = actionBuffer.components.back();
            comp.id = id;
            comp.typeId = type->id();
            comp.startData = std::shared_ptr<uint8_t[]>(new uint8_t[type->size()]);
            type->copy(value, comp.startData.get());
            comp.endData = std::shared_ptr<uint8_t[]>(new uint8_t[type->size()]);
            type->copy(value, comp.endData.get());
        }
    }

    void Undo::clearActions() {
        actions.clear();
        actionBuffer.undos.clear();
        actionBuffer.redos.clear();
        actionBuffer.components.clear();
        nextAction = 0;
        inAction = false;
    }

}
