//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Undo.h"
#include "Editor.h"
#include "tridot/engine/Engine.h"

namespace tridot {

    void Undo::addCustomAction(const std::function<void()>& undo, const std::function<void()>& redo){
        actionBuffer.undos.push_back(undo);
        actionBuffer.redos.push_back(redo);
    }

    void Undo::undoAction(){
        if(nextAction - 1 < actions.size()){
            for(auto &undo : actions[nextAction-1].undos){
                if(undo){
                    undo();
                }
            }
            for(auto &comp : actions[nextAction-1].components){
                ecs::EntityId id = comp.id;
                if(!engine.exists(id)){
                    id = engine.createHinted(id);
                }
                if(!engine.has(id, comp.typeId)) {
                    engine.addReflect(id, comp.typeId);
                }
                void *data = engine.get(id, comp.typeId);
                ecs::Reflection::get(comp.typeId).copy(comp.startData.get(), data);
            }
            nextAction--;
        }
    }

    void Undo::redoAction(){
        if(nextAction < actions.size()){
            for(auto &redo : actions[nextAction].redos){
                if(redo){
                    redo();
                }
            }
            for(auto &comp : actions[nextAction].components){
                if(engine.has(comp.id, comp.typeId)){
                    void *data = engine.get(comp.id, comp.typeId);
                    ecs::Reflection::get(comp.typeId).copy(comp.endData.get(), data);
                }
            }
            nextAction++;
        }
    }

    void Undo::beginAction() {
        inAction = true;
    }

    void Undo::endAction() {
        if(actionBuffer.undos.size() > 0 || actionBuffer.redos.size() > 0 || actionBuffer.components.size() > 0){
            while(nextAction < actions.size()){
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

    void Undo::destroyEntity(ecs::EntityId id) {
        for(auto &type : ecs::Reflection::getTypes()){
            if(engine.has(id, type->id())){
                changeComponent(id, type, engine.get(id, type->id()));
            }
        }
        addCustomAction(nullptr, [id](){
            engine.destroy(id);
        });
        if(!inAction){
            endAction();
        }
    }

    void Undo::duplicateEntity(ecs::EntityId id, ecs::EntityId newId) {
        addCustomAction([newId]() {
            engine.destroy(newId);
        }, [id]() {
            Editor::selection.duplicate(id, false);
        });
        if(!inAction){
            endAction();
        }
    }

    void Undo::changeComponent(ecs::EntityId id, ecs::Reflection::Type *type, void *value) {
        for(auto &comp : actionBuffer.components){
            if(comp.id == id && comp.typeId == type->id()){
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

    void Undo::clearActions() {
        actions.clear();
        actionBuffer.undos.clear();
        actionBuffer.redos.clear();
        actionBuffer.components.clear();
        nextAction = 0;
        inAction = false;
    }

}
